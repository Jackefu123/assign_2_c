#include "memory_manager.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

/**
 * @brief Structure representing a block of memory in the pool
 * @details
 * - offset: Position in the memory pool where the block starts
 * - size: Size of the block in bytes
 * - is_free: Flag indicating if the block is available for allocation
 * - next: Pointer to the next block in the linked list
 */
typedef struct MemBlock {
    size_t offset;         // Offset from the start of the memory pool
    size_t size;           // Size of the memory block
    int is_free;           // Flag indicating if the block is free
    struct MemBlock* next; // Pointer to the next memory block
} MemBlock;

// Global variables for the memory manager
static char* memory_pool = NULL;    // Pointer to the memory pool
static size_t pool_size = 0;        // Total size of the memory pool
static MemBlock* block_list = NULL; // Linked list of memory blocks

// Mutex for thread-safety
static pthread_mutex_t memory_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Initializes the memory manager with a pool of specified size
 */
void mem_init(size_t size) {
    // Step 1: Lock the mutex to ensure thread safety
    pthread_mutex_lock(&memory_lock);

    // Step 2: Allocate the memory pool using malloc
    memory_pool = (char*)malloc(size);
    // Step 3: Check if memory pool allocation succeeded
    if (!memory_pool) {
        fprintf(stderr, "Error: Could not allocate memory pool\n");
        pthread_mutex_unlock(&memory_lock);
        exit(EXIT_FAILURE);
    }

    // Step 4: Set the pool size
    pool_size = size;

    // Step 5: Allocate the initial block list node
    block_list = (MemBlock*)malloc(sizeof(MemBlock));
    // Step 6: Check if block list allocation succeeded
    if (!block_list) {
        fprintf(stderr, "Error: Could not allocate block list\n");
        free(memory_pool);
        pthread_mutex_unlock(&memory_lock);
        exit(EXIT_FAILURE);
    }

    // Step 7: Initialize the block list with one free block
    block_list->offset = 0;
    block_list->size = size;
    block_list->is_free = 1;
    block_list->next = NULL;

    // Step 8: Unlock the mutex
    pthread_mutex_unlock(&memory_lock);
}

/**
 * @brief Allocates a block of memory from the pool
 */
void* mem_alloc(size_t size) {
    // Step 1: Lock the mutex to ensure thread safety
    pthread_mutex_lock(&memory_lock);

    // Step 2: Handle zero-size allocation case
    if (size == 0) {
        pthread_mutex_unlock(&memory_lock);
        return memory_pool;
    }

    // Step 3: Search for a suitable free block using first-fit strategy
    MemBlock* current = block_list;
    while (current) {
        if (current->is_free && current->size >= size) {
            // Step 4: If a suitable block is found
            if (current->size > size) {
                // If block is larger than requested size, split it
                MemBlock* new_block = (MemBlock*)malloc(sizeof(MemBlock));
                if (!new_block) {
                    pthread_mutex_unlock(&memory_lock);
                    return NULL;
                }
                new_block->offset = current->offset + size;
                new_block->size = current->size - size;
                new_block->is_free = 1;
                new_block->next = current->next;

                current->size = size;
                current->is_free = 0;
                current->next = new_block;
            } else {
                // Mark the block as allocated
                current->is_free = 0;
            }
            // Return pointer to allocated memory
            void* result = memory_pool + current->offset;
            pthread_mutex_unlock(&memory_lock);
            return result;
        }
        current = current->next;
    }

    // Step 5: If no suitable block is found, return NULL
    pthread_mutex_unlock(&memory_lock);
    return NULL;
}

/**
 * @brief Frees a previously allocated block of memory
 */
void mem_free(void* ptr) {
    // Step 1: Check if pointer is NULL
    if (!ptr) return;

    // Step 2: Lock the mutex to ensure thread safety
    pthread_mutex_lock(&memory_lock);

    // Step 3: Calculate offset of the block to free
    size_t offset = (char*)ptr - memory_pool;

    // Step 4: Find the block in the block list
    MemBlock* current = block_list;
    MemBlock* prev = NULL;

    while (current) {
        if (current->offset == offset) {
            // Step 5: If block is found
            if (current->is_free) {
                pthread_mutex_unlock(&memory_lock);
                return;
            }
            // Mark it as free
            current->is_free = 1;

            // Merge with next block if it's free
            if (current->next && current->next->is_free) {
                MemBlock* next_block = current->next;
                current->size += next_block->size;
                current->next = next_block->next;
                free(next_block);
            }

            // Merge with previous block if it's free
            if (prev && prev->is_free) {
                prev->size += current->size;
                prev->next = current->next;
                free(current);
            }
            pthread_mutex_unlock(&memory_lock);
            return;
        }
        prev = current;
        current = current->next;
    }
    pthread_mutex_unlock(&memory_lock);
}

/**
 * @brief Resizes an allocated block of memory
 */
void* mem_resize(void* ptr, size_t size) {
    // Step 1: Handle NULL pointer case
    if (!ptr) return mem_alloc(size);

    // Step 2: Handle zero size case
    if (size == 0) {
        mem_free(ptr);
        return NULL;
    }

    // Step 3: Lock the mutex to ensure thread safety
    pthread_mutex_lock(&memory_lock);

    // Step 4: Find the block to resize
    MemBlock* current = block_list;
    size_t offset = (char*)ptr - memory_pool;
    while (current) {
        if (current->offset == offset) {
            // Step 5: If block is found
            if (current->size >= size) {
                // If new size is smaller, split the block
                if (current->size > size) {
                    MemBlock* new_block = (MemBlock*)malloc(sizeof(MemBlock));
                    if (!new_block) {
                        pthread_mutex_unlock(&memory_lock);
                        return NULL;
                    }
                    new_block->offset = current->offset + size;
                    new_block->size = current->size - size;
                    new_block->is_free = 1;
                    new_block->next = current->next;

                    current->size = size;
                    current->next = new_block;
                }
                pthread_mutex_unlock(&memory_lock);
                return ptr;
            } else {
                // If new size is larger, allocate new block and copy data
                pthread_mutex_unlock(&memory_lock);
                void* new_ptr = mem_alloc(size);
                if (new_ptr) {
                    memcpy(new_ptr, ptr, current->size);
                    mem_free(ptr);
                }
                return new_ptr;
            }
        }
        current = current->next;
    }
    pthread_mutex_unlock(&memory_lock);
    return NULL;
}

/**
 * @brief Deinitializes the memory manager and frees all resources
 */
void mem_deinit() {
    // Step 1: Lock the mutex to ensure thread safety
    pthread_mutex_lock(&memory_lock);

    // Step 2: Free the memory pool if it exists
    if (memory_pool) {
        free(memory_pool);
        memory_pool = NULL;
        pool_size = 0;
    }

    // Step 3: Free all block metadata
    MemBlock* current = block_list;
    while (current) {
        MemBlock* next = current->next;
        free(current);
        current = next;
    }

    // Step 4: Reset global variables
    block_list = NULL;

    // Step 5: Unlock the mutex
    pthread_mutex_unlock(&memory_lock);
}

