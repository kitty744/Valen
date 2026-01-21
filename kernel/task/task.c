#include <valen/task.h>
#include <valen/heap.h>
#include <valen/stdio.h>
#include <valen/string.h>
#include <valen/spinlock.h>

// Global task management
task_t *current_task = NULL;
static task_t *runqueue = NULL;
static pid_t next_pid = 1;
static spinlock_t runqueue_lock = SPINLOCK_INIT;
static spinlock_t current_task_lock = SPINLOCK_INIT;
static volatile uint8_t need_schedule = 0;
static volatile uint8_t tasks_exist = 0;

// Assembly context switch function
extern void switch_to(task_context_t *prev, task_context_t *next);

/**
 * @brief Initialize the task scheduler
 */
void scheduler_init(void) {
    current_task = NULL;
    runqueue = NULL;
    next_pid = 1;
    need_schedule = 0;
    tasks_exist = 0;
}

/**
 * @brief Add task to runqueue
 */
void add_task_to_runqueue(task_t *task) {
    spinlock_acquire(&runqueue_lock);
    
    if (!runqueue) {
        runqueue = task;
        task->next = task;
        task->prev = task;
    } else {
        // Insert at head of circular doubly-linked list
        task->next = runqueue;
        task->prev = runqueue->prev;
        runqueue->prev->next = task;
        runqueue->prev = task;
        runqueue = task;
    }
    
    tasks_exist = 1;  // Set flag that tasks exist
    spinlock_release(&runqueue_lock);
}

/**
 * @brief Remove task from runqueue
 */
void remove_task_from_runqueue(task_t *task) {
    spinlock_acquire(&runqueue_lock);
    
    if (!task || !runqueue) {
        spinlock_release(&runqueue_lock);
        return;
    }
    
    if (task->next == task) {
        // Only task in queue
        runqueue = NULL;
        tasks_exist = 0;  // No tasks exist
    } else {
        task->prev->next = task->next;
        task->next->prev = task->prev;
        if (runqueue == task) {
            runqueue = task->next;
        }
    }
    task->next = NULL;
    task->prev = NULL;
    
    spinlock_release(&runqueue_lock);
}

/**
 * @brief Create a new task
 */
task_t *task_create(void (*func)(void), const char *name) {
    task_t *task = (task_t*)malloc(sizeof(task_t));
    if (!task) {
        return NULL;
    }
    
    // Initialize task structure
    memset(task, 0, sizeof(task_t));
    
    task->pid = next_pid++;
    task->state = TASK_RUNNING;
    task->prio = 120;
    task->static_prio = 120;
    task->normal_prio = 120;
    task->rt_priority = 0;
    task->flags = TASK_RUNNING_FLAG;
    task->task_func = func;
    task->exit_code = 0;
    task->parent = current_task;
    
    // Copy command name
    if (name) {
        strncpy(task->comm, name, sizeof(task->comm) - 1);
        task->comm[sizeof(task->comm) - 1] = '\0';
    } else {
        strcpy(task->comm, "unknown");
    }
    
    // Allocate kernel stack
    task->stack_size = 8192;
    task->stack = malloc(task->stack_size);
    if (!task->stack) {
        free(task);
        return NULL;
    }
    
    // Set up initial stack for new task
    uint64_t *stack_top = (uint64_t*)((uint8_t*)task->stack + task->stack_size);
    
    // Align stack to 16-byte boundary
    stack_top = (uint64_t*)((uint64_t)stack_top & ~0xF);
    
    // Push initial values that context switch expects to pop
    // The context switch will pop: r15, r14, r13, r12, rbx, rbp
    *--stack_top = 0;  // r15
    *--stack_top = 0;  // r14  
    *--stack_top = 0;  // r13
    *--stack_top = 0;  // r12
    *--stack_top = 0;  // rbx
    *--stack_top = 0;  // rbp
    
    // Set up context structure
    task->context.rsp = (uint64_t)stack_top;
    task->context.rip = (uint64_t)func;
    task->context.cs = 0x08;
    task->context.ss = 0x10;
    task->context.eflags = 0x202;
    
    // Zero out all general purpose registers
    task->context.r15 = 0;
    task->context.r14 = 0;
    task->context.r13 = 0;
    task->context.r12 = 0;
    task->context.rbp = 0;
    task->context.rbx = 0;
    task->context.r11 = 0;
    task->context.r10 = 0;
    task->context.r9 = 0;
    task->context.r8 = 0;
    task->context.rax = 0;
    task->context.rcx = 0;
    task->context.rdx = 0;
    task->context.rsi = 0;
    task->context.rdi = 0;
    task->context.orig_rax = 0;
    
    // Add to runqueue
    add_task_to_runqueue(task);
    
    return task;
}

/**
 * @brief Exit current task
 */
void task_exit(long exit_code) {
    spinlock_acquire(&current_task_lock);
    
    if (!current_task) {
        spinlock_release(&current_task_lock);
        return;
    }
    
    printf("Task '%s' (PID %d) exiting with code %ld\n", 
           current_task->comm, current_task->pid, exit_code);
    
    current_task->state = TASK_ZOMBIE;
    current_task->exit_code = exit_code;
    
    // Save current_task pointer before releasing lock
    task_t *exiting_task = current_task;
    
    spinlock_release(&current_task_lock);
    
    // Remove from runqueue and schedule next task
    remove_task_from_runqueue(exiting_task);
    schedule();
}

/**
 * @brief Core scheduler
 */
void schedule(void) {
    spinlock_acquire(&runqueue_lock);
    spinlock_acquire(&current_task_lock);
    
    if (!runqueue) {
        spinlock_release(&current_task_lock);
        spinlock_release(&runqueue_lock);
        return;
    }
    
    task_t *next = NULL;
    task_t *old_current = current_task;
    
    // Find next runnable task
    if (!current_task) {
        next = runqueue;
    } else {
        next = current_task->next;
        if (next == current_task) {
            next = current_task; // Only one task
        }
    }
    
    if (next && next != current_task) {
        // Update current_task while holding both locks
        current_task = next;
        
        // Release both locks before context switch
        spinlock_release(&current_task_lock);
        spinlock_release(&runqueue_lock);
        
        // Perform context switch
        if (old_current) {
            switch_to(&old_current->context, &next->context);
        } else {
            // First task - set up stack and jump directly
            asm volatile (
                "mov %0, %%rsp\n"
                "jmp *%1"
                :
                : "r" (next->context.rsp), "r" (next->context.rip)
                : "memory"
            );
        }
    } else {
        spinlock_release(&current_task_lock);
        spinlock_release(&runqueue_lock);
    }
}

/**
 * @brief Timer tick handler for scheduler
 */
void scheduler_tick(void) {
    // Don't acquire locks in interrupt context
    // Just check if tasks exist and set scheduling flag
    
    if (!tasks_exist) return;
    
    // Simple time slice management
    static int counter = 0;
    counter++;
    
    // Schedule every 25 ticks (0.5 seconds at 50Hz)
    if (counter >= 25) {
        counter = 0;
        need_schedule = 1;  // Set flag for deferred scheduling
    }
}

/**
 * @brief Get current task
 */
task_t *get_current_task(void) {
    spinlock_acquire(&current_task_lock);
    task_t *task = current_task;
    spinlock_release(&current_task_lock);
    return task;
}

/**
 * @brief Get current PID
 */
pid_t get_current_pid(void) {
    spinlock_acquire(&current_task_lock);
    pid_t pid = current_task ? current_task->pid : -1;
    spinlock_release(&current_task_lock);
    return pid;
}

/**
 * @brief Yield CPU to next task
 */
void yield(void) {
    if (need_schedule) {
        need_schedule = 0;
        schedule();
    }
}

/**
 * @brief Find task by PID
 */
task_t *find_task_by_pid(pid_t pid) {
    if (pid <= 0) return NULL;
    
    spinlock_acquire(&runqueue_lock);
    
    if (!runqueue) {
        spinlock_release(&runqueue_lock);
        return NULL;
    }
    
    task_t *current = runqueue;
    task_t *found = NULL;
    
    do {
        if (current->pid == pid) {
            found = current;
            break;
        }
        current = current->next;
    } while (current != runqueue);
    
    spinlock_release(&runqueue_lock);
    return found;
}

/**
 * @brief Kill a task by PID
 */
int kill_task(pid_t pid) {
    if (pid <= 0) return -1;
    
    spinlock_acquire(&runqueue_lock);
    
    task_t *target = NULL;
    task_t *current = runqueue;
    
    if (runqueue) {
        do {
            if (current->pid == pid) {
                target = current;
                break;
            }
            current = current->next;
        } while (current != runqueue);
    }
    
    if (!target) {
        spinlock_release(&runqueue_lock);
        return -1;
    }
    
    // Don't allow killing current task
    if (target == current_task) {
        spinlock_release(&runqueue_lock);
        return -2;
    }
    
    // Mark as zombie and remove from runqueue
    target->state = TASK_ZOMBIE;
    
    if (target->next == target) {
        // Only task in queue
        runqueue = NULL;
    } else {
        target->prev->next = target->next;
        target->next->prev = target->prev;
        if (runqueue == target) {
            runqueue = target->next;
        }
    }
    target->next = NULL;
    target->prev = NULL;
    
    spinlock_release(&runqueue_lock);
    
    // Free the task's resources (safe to do outside lock)
    if (target->stack) {
        free(target->stack);
    }
    free(target);
    
    return 0;
}
