
#include <valen/pmm.h>
#include <valen/paging.h>
#include <valen/spinlock.h>

/* The offset used to access physical memory in the higher half */
#define KERNEL_VIRT_OFFSET 0xFFFFFFFF80000000
#define PHYS_TO_VIRT(p) ((void *)((uint64_t)(p) + KERNEL_VIRT_OFFSET))
#define VIRT_TO_PHYS(v) ((uint64_t)(v) - KERNEL_VIRT_OFFSET)

static uint8_t *bitmap;
static uint64_t bitmap_size;
static uint64_t total_pages;
static uint64_t used_pages;
static spinlock_t pmm_lock = SPINLOCK_INIT;

/**
 * @brief Initializes the PMM bitmap.
 * @param start The VIRTUAL address where the bitmap should be placed.
 * @param size The total size of PHYSICAL RAM in bytes.
 */
void pmm_init(uintptr_t start, uint64_t size)
{
    /* The bitmap pointer is a virtual address in the higher half */
    bitmap = (uint8_t *)start;
    total_pages = size / 4096;
    bitmap_size = (total_pages + 7) / 8;

    /* Initially mark everything as USED until kmain calls pmm_mark_free */
    used_pages = total_pages;

    /* Fill bitmap with 0xFF (all used) */
    for (uint64_t i = 0; i < bitmap_size; i++)
    {
        bitmap[i] = 0xFF;
    }
}

/**
 * @brief Marks a PHYSICAL address as free in the bitmap.
 */
void pmm_mark_free(uintptr_t addr)
{
    spinlock_acquire(&pmm_lock);
    
    uint64_t block = addr / 4096;
    if (block < total_pages)
    {
        if (bitmap[block / 8] & (1 << (block % 8)))
        {
            bitmap[block / 8] &= ~(1 << (block % 8));
            if (used_pages > 0)
                used_pages--;
        }
    }
    
    spinlock_release(&pmm_lock);
}

/**
 * @brief Marks a PHYSICAL address as used in the bitmap.
 */
void pmm_mark_used(uintptr_t addr)
{
    spinlock_acquire(&pmm_lock);
    
    uint64_t block = addr / 4096;
    if (block < total_pages)
    {
        if (!(bitmap[block / 8] & (1 << (block % 8))))
        {
            bitmap[block / 8] |= (1 << (block % 8));
            used_pages++;
        }
    }
    
    spinlock_release(&pmm_lock);
}

/**
 * @brief Finds a free physical frame and returns its PHYSICAL address.
 */
void *pmm_alloc_page()
{
    spinlock_acquire(&pmm_lock);
    
    for (uint64_t i = 0; i < bitmap_size; i++)
    {
        if (bitmap[i] != 0xFF)
        {
            for (int j = 0; j < 8; j++)
            {
                if (!(bitmap[i] & (1 << j)))
                {
                    uintptr_t addr = (i * 8 + j) * 4096;

                    /* Safety: Never allocate bottom 2MB (Kernel/BIOS/Page Tables) */
                    /* Note: These are physical address checks */
                    if (addr < 0x200000)
                        continue;

                    bitmap[i] |= (1 << j);
                    used_pages++;

                    spinlock_release(&pmm_lock);
                    /* Return virtual address that can be used by kernel */
                    return PHYS_TO_VIRT(addr);
                }
            }
        }
    }
    
    spinlock_release(&pmm_lock);
    return 0;
}

/**
 * @brief Allocates multiple contiguous physical pages
 */
void *pmm_alloc_pages(uint64_t count)
{
    spinlock_acquire(&pmm_lock);
    
    // Find enough contiguous free pages
    for (uint64_t i = 0; i < bitmap_size; i++) {
        if (bitmap[i] != 0xFF) {
            // Check if we have enough contiguous pages starting here
            uint64_t found = 0;
            for (int j = 0; j < 8 && found < count; j++) {
                if (!(bitmap[i] & (1 << j))) {
                    found++;
                } else {
                    break;
                }
            }
            
            if (found >= count) {
                // Found enough contiguous pages
                uintptr_t start_addr = (i * 8 + (8 - found)) * 4096;
                
                // Mark all pages as used
                for (uint64_t k = 0; k < count; k++) {
                    uint64_t page_idx = (start_addr / 4096) + k;
                    uint64_t byte_idx = page_idx / 8;
                    uint64_t bit_idx = page_idx % 8;
                    bitmap[byte_idx] |= (1 << bit_idx);
                    used_pages++;
                }
                
                spinlock_release(&pmm_lock);
                return PHYS_TO_VIRT(start_addr);
            }
        }
    }
    
    spinlock_release(&pmm_lock);
    return 0;
}

/**
 * @brief Frees a page given its PHYSICAL address.
 */
void pmm_free_page(void *addr)
{
    pmm_mark_free((uintptr_t)addr);
}

uint64_t pmm_get_total_kb() { return total_pages * 4ULL; }
uint64_t pmm_get_used_kb() { return used_pages * 4ULL; }
uint64_t pmm_get_free_kb() { return (total_pages > used_pages) ? (total_pages - used_pages) * 4ULL : 0; }