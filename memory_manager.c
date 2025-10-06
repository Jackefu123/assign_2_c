#include "memory_manager.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

typedef struct MemBlock {
    size_t offset;         
    size_t size;           
    int is_free;           
    struct MemBlock* next; 
} MemBlock;

static char* memory_pool = NULL;    
static size_t pool_size = 0;        
static MemBlock* block_list = NULL; 
static pthread_mutex_t memory_lock = PTHREAD_MUTEX_INITIALIZER;

void mem_init(size_t size) {
    pthread_mutex_lock(&memory_lock);

    memory_pool = (char*)malloc(size);
    if (!memory_pool) {
        fprintf(stderr, "Error: Could not allocate memory pool\n");
        pthread_mutex_unlock(&memory_lock);
        exit(EXIT_FAILURE);
    }

    pool_size = size;
    
    block_list = (MemBlock*)malloc(sizeof(MemBlock));
    if (!block_list) {
        fprintf(stderr, "Error: Could not allocate block list\n");
        free(memory_pool);
        pthread_mutex_unlock(&memory_lock);
        exit(EXIT_FAILURE);
    }

    block_list->offset = 0;
    block_list->size = size;
    block_list->is_free = 1;
    block_list->next = NULL;

    pthread_mutex_unlock(&memory_lock);
}

void* mem_alloc(size_t size) {
    pthread_mutex_lock(&memory_lock);

    if (size == 0) {
        pthread_mutex_unlock(&memory_lock);
        return memory_pool;
    }

    MemBlock* current = block_list;
    while (current) {
        if (current->is_free && current->size >= size) {
            current->is_free = 0;
            
            if (current->size > size) {
                MemBlock* new_block = (MemBlock*)malloc(sizeof(MemBlock));
                if (new_block) {
                    new_block->offset = current->offset + size;
                    new_block->size = current->size - size;
                    new_block->is_free = 1;
                    new_block->next = current->next;
                    
                    current->size = size;
                    current->next = new_block;
                }
            }
            
            void* result = memory_pool + current->offset;
            pthread_mutex_unlock(&memory_lock);
            return result;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&memory_lock);
    return NULL;
}

void mem_free(void* ptr) {
    if (!ptr) return;

    pthread_mutex_lock(&memory_lock);

    size_t offset = (char*)ptr - memory_pool;
    MemBlock* current = block_list;
    MemBlock* prev = NULL;

    while (current) {
        if (current->offset == offset) {
            current->is_free = 1;
            
            // Coalesce with next block if free
            if (current->next && current->next->is_free && 
                current->offset + current->size == current->next->offset) {
                MemBlock* next_block = current->next;
                current->size += next_block->size;
                current->next = next_block->next;
                free(next_block);
            }
            
            // Coalesce with previous block if free
            if (prev && prev->is_free && 
                prev->offset + prev->size == current->offset) {
                prev->size += current->size;
                prev->next = current->next;
                free(current);
            }
            
            break;
        }
        prev = current;
        current = current->next;
    }
    pthread_mutex_unlock(&memory_lock);
}

void* mem_resize(void* ptr, size_t size) {
    if (!ptr) return mem_alloc(size);

    if (size == 0) {
        mem_free(ptr);
        return NULL;
    }

    pthread_mutex_lock(&memory_lock);

    size_t offset = (char*)ptr - memory_pool;
    MemBlock* current = block_list;
    
    while (current) {
        if (current->offset == offset) {
            if (current->size >= size) {
                // Current block is large enough
                if (current->size > size) {
                    // Split if significantly larger
                    MemBlock* new_block = (MemBlock*)malloc(sizeof(MemBlock));
                    if (new_block) {
                        new_block->offset = current->offset + size;
                        new_block->size = current->size - size;
                        new_block->is_free = 1;
                        new_block->next = current->next;
                        
                        current->size = size;
                        current->next = new_block;
                    }
                }
                pthread_mutex_unlock(&memory_lock);
                return ptr;
            } else {
                // Need to allocate new block and copy
                size_t old_size = current->size;
                pthread_mutex_unlock(&memory_lock);
                
                void* new_ptr = mem_alloc(size);
                if (new_ptr) {
                    memcpy(new_ptr, ptr, old_size);
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

void mem_deinit() {
    pthread_mutex_lock(&memory_lock);

    if (memory_pool) {
        free(memory_pool);
        memory_pool = NULL;
    }

    MemBlock* current = block_list;
    while (current) {
        MemBlock* next = current->next;
        free(current);
        current = next;
    }

    block_list = NULL;
    pool_size = 0;

    pthread_mutex_unlock(&memory_lock);
}

