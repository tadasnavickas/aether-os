#include "heap.h"
#include "pmm.h"

#define HEAP_INITIAL_PAGES 64

struct heap_block {
    size_t size;
    int is_free;
    struct heap_block *next;
    struct heap_block *prev;
};

#define BLOCK_HEADER_SIZE sizeof(struct heap_block)

static struct heap_block *heap_head = NULL;
static size_t heap_used = 0;
static size_t heap_total = 0;

void heap_init(void) {
    heap_head = NULL;
    heap_used = 0;
    heap_total = 0;

    for (size_t i = 0; i < HEAP_INITIAL_PAGES; i++) {
        void *page_phys = pmm_alloc_page();
        if (!page_phys) break;

        struct heap_block *blk = (struct heap_block *)phys_to_virt(page_phys);
        blk->size = PAGE_SIZE - BLOCK_HEADER_SIZE;
        blk->is_free = 1;
        blk->next = heap_head;
        blk->prev = NULL;

        if (heap_head) {
            heap_head->prev = blk;
        }
        heap_head = blk;
        heap_total += PAGE_SIZE;
    }
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    size = (size + 7) & ~((size_t)7);

    struct heap_block *curr = heap_head;
    while (curr) {
        if (curr->is_free && curr->size >= size) {
            if (curr->size >= size + BLOCK_HEADER_SIZE + 16) {
                struct heap_block *new_block = (struct heap_block *)((uint8_t *)curr + BLOCK_HEADER_SIZE + size);
                new_block->size = curr->size - size - BLOCK_HEADER_SIZE;
                new_block->is_free = 1;
                new_block->next = curr->next;
                new_block->prev = curr;

                if (curr->next) {
                    curr->next->prev = new_block;
                }
                curr->next = new_block;
                curr->size = size;
            }

            curr->is_free = 0;
            heap_used += curr->size + BLOCK_HEADER_SIZE;
            return (void *)((uint8_t *)curr + BLOCK_HEADER_SIZE);
        }
        curr = curr->next;
    }

    return NULL;
}

void kfree(void *ptr) {
    if (!ptr) return;

    struct heap_block *block = (struct heap_block *)((uint8_t *)ptr - BLOCK_HEADER_SIZE);
    block->is_free = 1;
    heap_used -= (block->size + BLOCK_HEADER_SIZE);

    if (block->next && block->next->is_free) {
        block->size += BLOCK_HEADER_SIZE + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    if (block->prev && block->prev->is_free) {
        block->prev->size += BLOCK_HEADER_SIZE + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

size_t heap_get_used(void) {
    return heap_used;
}

size_t heap_get_total(void) {
    return heap_total;
}