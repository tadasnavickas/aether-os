#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>

void heap_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);

size_t heap_get_used(void);
size_t heap_get_total(void);

#endif