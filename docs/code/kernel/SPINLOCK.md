# Spinlock API

Spinlocks are low-level synchronization primitives used to protect shared resources from concurrent access in a multi-tasking environment. They implement busy-wait locking where a thread continuously checks until the lock becomes available.

## When to Use Spinlocks

**Use spinlocks for:**

- Short critical sections where the lock will be held briefly
- Protecting shared data structures in interrupt handlers
- Low-level kernel synchronization where blocking is not acceptable
- Situations where the lock contention is expected to be minimal

**Do NOT use spinlocks for:**

- Long-running operations that might block
- Operations that might sleep or yield the CPU
- User-space code (use higher-level synchronization primitives)
- Situations with high contention where other synchronization methods would be more efficient

## API Reference

### Data Structures

```c
typedef struct {
    volatile uint32_t lock;
} spinlock_t;
```

### Initialization

```c
#define SPINLOCK_INIT { .lock = 0 }

void spinlock_init(spinlock_t *lock);
```

- `SPINLOCK_INIT`: Static initializer for spinlock variables
- `spinlock_init()`: Dynamically initialize a spinlock to unlocked state

### Lock Operations

```c
void spinlock_acquire(spinlock_t *lock);
void spinlock_release(spinlock_t *lock);
uint8_t spinlock_try_acquire(spinlock_t *lock);
```

- `spinlock_acquire()`: Acquire the lock, blocking until successful
- `spinlock_release()`: Release the lock, allowing other threads to acquire it
- `spinlock_try_acquire()`: Attempt to acquire the lock without blocking (returns 1 on success, 0 on failure)

## Usage Examples

### Basic Usage

```c
spinlock_t my_lock = SPINLOCK_INIT;
int shared_counter = 0;

void increment_counter(void) {
    spinlock_acquire(&my_lock);
    shared_counter++;
    spinlock_release(&my_lock);
}
```

### Non-blocking Attempt

```c
int try_update_resource(void) {
    if (spinlock_try_acquire(&my_lock)) {
        // Successfully acquired lock
        update_shared_resource();
        spinlock_release(&my_lock);
        return 1; // Success
    }
    return 0; // Lock was busy
}
```

## Implementation Details

The spinlock implementation uses atomic compare-and-swap (CMPXCHG) instructions to ensure thread-safe operation:

- **Acquisition**: Uses `lock cmpxchgl` with a pause instruction to reduce power consumption during busy-wait
- **Release**: Simple atomic store of 0 to unlock
- **Memory Barriers**: Assembly constraints ensure proper memory ordering

## Important Notes

- Spinlocks disable interrupts implicitly on x86 through the `lock` prefix
- Always release locks in the same scope where they were acquired
- Never call functions that might sleep while holding a spinlock
- Be aware of potential deadlocks if multiple locks are acquired in different orders

## Performance Considerations

- Spinlocks are most efficient when lock hold times are very short (microseconds)
- The `pause` instruction helps reduce power consumption and improve performance on hyperthreaded CPUs
- Consider using lock-free algorithms or other synchronization primitives for high-contention scenarios
