#include "pmm.h"
#include "../limine.h"

__attribute__((used, section(".requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

static uint8_t *bitmap = NULL;
static uint64_t bitmap_size = 0;
static uint64_t total_pages = 0;
static uint64_t free_pages = 0;
static uint64_t hhdm_offset = 0;

static inline void bitmap_set(uint64_t page) {
    bitmap[page / 8] |= (1 << (page % 8));
}

static inline void bitmap_clear(uint64_t page) {
    bitmap[page / 8] &= ~(1 << (page % 8));
}

static inline int bitmap_test(uint64_t page) {
    return (bitmap[page / 8] >> (page % 8)) & 1;
}

void pmm_init(void) {
    if (!memmap_request.response || !hhdm_request.response) {
        return;
    }

    hhdm_offset = hhdm_request.response->offset;
    struct limine_memmap_response *memmap = memmap_request.response;

    uint64_t highest_addr = 0;

    for (size_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t top = entry->base + entry->length;
            if (top > highest_addr) {
                highest_addr = top;
            }
        }
    }

    total_pages = highest_addr / PAGE_SIZE;
    bitmap_size = (total_pages + 7) / 8;

    for (size_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= bitmap_size) {
            bitmap = (uint8_t *)(entry->base + hhdm_offset);
            break;
        }
    }

    for (uint64_t i = 0; i < bitmap_size; i++) {
        bitmap[i] = 0xFF;
    }

    for (size_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            for (uint64_t addr = entry->base; addr < entry->base + entry->length; addr += PAGE_SIZE) {
                bitmap_clear(addr / PAGE_SIZE);
                free_pages++;
            }
        }
    }

    uint64_t bitmap_phys_addr = (uint64_t)bitmap - hhdm_offset;
    for (uint64_t addr = bitmap_phys_addr; addr < bitmap_phys_addr + bitmap_size; addr += PAGE_SIZE) {
        bitmap_set(addr / PAGE_SIZE);
        if (free_pages > 0) free_pages--;
    }
}

void *pmm_alloc_page(void) {
    for (uint64_t i = 0; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            free_pages--;
            return (void *)(i * PAGE_SIZE);
        }
    }
    return NULL;
}

void pmm_free_page(void *addr) {
    uint64_t page = (uint64_t)addr / PAGE_SIZE;
    if (page < total_pages && bitmap_test(page)) {
        bitmap_clear(page);
        free_pages++;
    }
}

uint64_t pmm_get_total_memory(void) {
    return total_pages * PAGE_SIZE;
}

uint64_t pmm_get_free_memory(void) {
    return free_pages * PAGE_SIZE;
}