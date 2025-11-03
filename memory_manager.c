#include "memory_manager.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

// Trådsäker minneshanterare med grovkornig låsning (coarse-grained locking)
// Implementerar trådsäkerhet genom att använda en enda mutex för alla minnesoperationer
// Detta garanterar att endast en tråd i taget kan modifiera minnesstrukturen

typedef struct MemBlock {
    size_t offset;         // Offset i minnespoolen där detta block börjar
    size_t size;           // Storleken på detta minnesblock i bytes
    int is_free;           // 1 om blocket är ledigt, 0 om det är allokerat
    struct MemBlock* next; // Pekare till nästa block i listan
} MemBlock;

// Globala variabler för minneshantering
static char* memory_pool = NULL;    // Pekare till den allokerade minnespoolen
static size_t pool_size = 0;        // Total storlek på minnespoolen
static MemBlock* block_list = NULL; // Länkad lista över alla minnesblock

// Trådsäkerhetsmekanism: Grovkornig låsning (coarse-grained locking)
// En enda mutex skyddar hela minneshanteraren för enkelhetens skull
// Fördel: Enkel att implementera och garanterar fullständig trådsäkerhet
// Nackdel: Kan bli en flaskhals vid hög samtidighet då endast en operation kan ske åt gången
static pthread_mutex_t memory_lock = PTHREAD_MUTEX_INITIALIZER;

// Initierar minneshanteraren med en pool av specificerad storlek
// Trådsäkerhet: Låser under hela initieringen för att förhindra samtidig initiering
// Kallas typiskt endast en gång vid programstart, så låsning påverkar inte prestanda
void mem_init(size_t size) {
    pthread_mutex_lock(&memory_lock); // Serialiserar initieringen - endast en tråd kan initiera åt gången

    memory_pool = (char*)malloc(size); // Allokerar en sammanhängande minnespool från systemet
    if (!memory_pool) {
        fprintf(stderr, "Error: Could not allocate memory pool\n");
        pthread_mutex_unlock(&memory_lock);
        exit(EXIT_FAILURE);
    }

    pool_size = size;
    
    // Skapar ett enda stort ledigt block som täcker hela poolen
    block_list = (MemBlock*)malloc(sizeof(MemBlock));
    if (!block_list) {
        fprintf(stderr, "Error: Could not allocate block list\n");
        free(memory_pool);
        pthread_mutex_unlock(&memory_lock); // Viktigt: lås upp även vid fel
        exit(EXIT_FAILURE);
    }

    // Initialiserar det första blocket som ledigt och täcker hela poolen
    block_list->offset = 0;
    block_list->size = size;
    block_list->is_free = 1;
    block_list->next = NULL;

    pthread_mutex_unlock(&memory_lock); // Låser upp efter lyckad initiering
}

// Allokerar ett minnesblock av angiven storlek
// Trådsäkerhet: Hela allokeringsprocessen är atomisk genom mutex-låsning
// Detta förhindrar race conditions när flera trådar söker och allokerar block samtidigt
// Använder first-fit strategi för att hitta lämpligt block
void* mem_alloc(size_t size) {
    pthread_mutex_lock(&memory_lock); // Kritisk sektion börjar - skyddar sökning och allokering

    // Gränskontroll: Hantera noll-storlek
    if (size == 0) {
        pthread_mutex_unlock(&memory_lock);
        return memory_pool; // Returnerar sentinelpekare för noll-storlek (använd/frigör ej)
    }

    // First-fit sökning: Hitta första lediga blocket som är tillräckligt stort
    MemBlock* current = block_list;
    while (current) {
        if (current->is_free && current->size >= size) {
            current->is_free = 0; // Atomisk markering: blocket är nu allokerat
            
            // Block-splitting: Om blocket är större än behövt, dela upp det
            if (current->size > size) {
                MemBlock* new_block = (MemBlock*)malloc(sizeof(MemBlock));
                if (new_block) {
                    // Skapa ett nytt ledigt block från återstående utrymme
                    new_block->offset = current->offset + size;
                    new_block->size = current->size - size;
                    new_block->is_free = 1;
                    new_block->next = current->next;
                    
                    // Uppdatera det aktuella blocket
                    current->size = size;
                    current->next = new_block;
                }
                // Om malloc misslyckas används hela blocket (inget splitting)
            }
            
            // Beräkna pekare till det allokerade området
            void* result = memory_pool + current->offset;
            pthread_mutex_unlock(&memory_lock); // Kritisk sektion slut - allokering lyckades
            return result;
        }
        current = current->next;
    }

    // Felhantering: Inget ledigt block med tillräcklig storlek hittades
    pthread_mutex_unlock(&memory_lock); // Kritisk sektion slut - allokering misslyckades
    return NULL;
}

// Frigör ett tidigare allokerat minnesblock
// Trådsäkerhet: Mutex skyddar både frigöring och sammanslagning (coalescing) av block
// Detta förhindrar konflikter med samtidiga allokeringar eller andra frigöringar
void mem_free(void* ptr) {
    // Gränskontroll: NULL-pekare är tillåten och gör ingenting (standard C-beteende)
    if (!ptr) return;

    pthread_mutex_lock(&memory_lock); // Kritisk sektion börjar - skyddar frigöring och coalescing

    // Hitta blocket genom att beräkna offset från poolens start
    size_t offset = (char*)ptr - memory_pool;
    MemBlock* current = block_list;
    MemBlock* prev = NULL;

    // Sök efter blocket som matchar denna offset
    while (current) {
        if (current->offset == offset) {
            current->is_free = 1; // Atomisk markering: blocket är nu ledigt
            
            // Coalescing (sammanslagning) framåt: Slå samman med nästa block om möjligt
            // Detta förhindrar fragmentering genom att kombinera intilliggande lediga block
            if (current->next && current->next->is_free && 
                current->offset + current->size == current->next->offset) {
                MemBlock* next_block = current->next;
                current->size += next_block->size;
                current->next = next_block->next;
                free(next_block); // Ta bort den överflödiga blocknoden
            }
            
            // Coalescing (sammanslagning) bakåt: Slå samman med föregående block om möjligt
            if (prev && prev->is_free && 
                prev->offset + prev->size == current->offset) {
                prev->size += current->size;
                prev->next = current->next;
                free(current); // Ta bort den överflödiga blocknoden
            }
            
            break; // Block hittat och friggjort
        }
        prev = current;
        current = current->next;
    }
    pthread_mutex_unlock(&memory_lock); // Kritisk sektion slut - frigöring och coalescing klart
}

// Ändrar storleken på ett tidigare allokerat block
// Trådsäkerhet: Komplex synkronisering eftersom funktionen kombinerar frigöring och allokering
// Låser först för att inspektera blocket, låser upp innan rekursiva anrop för att undvika deadlock
void* mem_resize(void* ptr, size_t size) {
    // Gränskontroll: NULL-pekare - beter sig som malloc
    if (!ptr) return mem_alloc(size);

    // Gränskontroll: Noll-storlek - beter sig som free
    if (size == 0) {
        mem_free(ptr);
        return NULL;
    }

    pthread_mutex_lock(&memory_lock); // Kritisk sektion börjar - inspekterar blocket

    // Hitta blocket som ska ändra storlek
    size_t offset = (char*)ptr - memory_pool;
    MemBlock* current = block_list;
    
    while (current) {
        if (current->offset == offset) {
            // Fall 1: Nuvarande block är tillräckligt stort (krympa eller behåll)
            if (current->size >= size) {
                // In-place shrinking: Dela upp blocket om det är större än behövt
                if (current->size > size) {
                    MemBlock* new_block = (MemBlock*)malloc(sizeof(MemBlock));
                    if (new_block) {
                        // Skapa ett ledigt block från överskottet
                        new_block->offset = current->offset + size;
                        new_block->size = current->size - size;
                        new_block->is_free = 1;
                        new_block->next = current->next;
                        
                        current->size = size;
                        current->next = new_block;
                    }
                }
                pthread_mutex_unlock(&memory_lock); // Kritisk sektion slut - resize på plats lyckades
                return ptr; // Samma pekare, ändrad storlek
            } else {
                // Fall 2: Nuvarande block är för litet - behöver allokera nytt och flytta data
                // VIKTIGT: Lås upp före mem_alloc/mem_free för att undvika deadlock
                size_t old_size = current->size;
                pthread_mutex_unlock(&memory_lock); // Måste låsa upp innan rekursiva anrop
                
                // Allokera nytt block (denna funktion låser internt)
                void* new_ptr = mem_alloc(size);
                if (new_ptr) {
                    memcpy(new_ptr, ptr, old_size); // Kopiera data från gammalt till nytt block
                    mem_free(ptr); // Frigör gammalt block (denna funktion låser internt)
                }
                return new_ptr; // Returnera ny pekare eller NULL vid fel
            }
        }
        current = current->next;
    }
    
    // Felhantering: Blocket hittades inte i listan
    pthread_mutex_unlock(&memory_lock); // Kritisk sektion slut
    return NULL;
}

// Stänger ner minneshanteraren och frigör alla resurser
// Trådsäkerhet: Låser under hela nedstängningen för att förhindra samtidig access
// Kallas typiskt endast en gång vid programavslut
void mem_deinit() {
    pthread_mutex_lock(&memory_lock); // Kritisk sektion börjar - serialiserar nedstängning

    // Frigör själva minnespoolen
    if (memory_pool) {
        free(memory_pool);
        memory_pool = NULL;
    }

    // Frigör alla blocknoder i den länkade listan
    MemBlock* current = block_list;
    while (current) {
        MemBlock* next = current->next;
        free(current);
        current = next;
    }

    // Återställ alla globala variabler
    block_list = NULL;
    pool_size = 0;

    pthread_mutex_unlock(&memory_lock); // Kritisk sektion slut - nedstängning klar
    // OBS: Mutex destrueras inte här (skulle kräva pthread_mutex_destroy)
}

