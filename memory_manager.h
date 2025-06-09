#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stddef.h> // For size_t

/**
 * Initializes the memory manager with a pool of specified size
 */
void mem_init(size_t size);

/**
 * Allocates a block of memory from the pool
 */
void* mem_alloc(size_t size);

/**
 * Frees a previously allocated block of memory
 */
void mem_free(void* block);

/**
 * Resizes an allocated block of memory
 */
void* mem_resize(void* block, size_t size);

/**
 * Deinitializes the memory manager and frees all resources
 */
void mem_deinit();

#endif
