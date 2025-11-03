#include "linked_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Trådsäker länkad lista med grovkornig låsning (coarse-grained locking)
// Implementerar trådsäkerhet genom att använda en enda mutex för alla listoperationer
// Detta garanterar att endast en tråd i taget kan modifiera eller läsa listan

// Trådsäkerhetsmekanism: Grovkornig låsning (coarse-grained locking)
// En enda mutex skyddar hela listan för enkelhetens skull
// Fördel: Enkel att implementera, garanterar fullständig konsistens
// Nackdel: Både läsare och skrivare konkurrerar om samma lås, kan begränsa prestanda
// Alternativ: Read-write locks skulle tillåta samtidiga läsningar
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
extern pthread_mutex_t list_mutex; // Redundant omdeklaration; behålls för kompatibilitet

// Initierar en tom länkad lista
// Trådsäkerhet: Låser under initiering trots att funktionen typiskt anropas en gång
// Detta förhindrar problem om flera trådar försöker initiera samtidigt
void list_init(Node** head, size_t size) {
    pthread_mutex_lock(&list_mutex); // Kritisk sektion börjar - skyddar initiering
    *head = NULL; // Tom lista
    (void)size; // Parameter används inte i denna implementation
    pthread_mutex_unlock(&list_mutex); // Kritisk sektion slut
}

// Lägger till en ny nod i slutet av listan
// Trådsäkerhet: Hela operationen är atomisk genom mutex-låsning
// Förhindrar race conditions när flera trådar försöker lägga till samtidigt
// TODO: Bör använda mem_alloc istället för malloc enligt uppgiftens krav
void list_insert(Node** head, uint16_t data) {
    pthread_mutex_lock(&list_mutex); // Kritisk sektion börjar - endast en skrivare åt gången
    
    // Allokera ny nod
    // TODO: Använd mem_alloc för att integrera med den anpassade minneshanteraren
    Node* new_node = (Node*)malloc(sizeof(Node));
    
    // Felhantering: Kontrollera om allokeringen lyckades
    if (!new_node) {
        fprintf(stderr, "Error: Memory allocation\n");
        pthread_mutex_unlock(&list_mutex); // VIKTIGT: Lås upp även vid fel
        return;
    }
    
    // Initialisera den nya noden
    new_node->data = data;
    new_node->next = NULL;
    
    // Fall 1: Tom lista - sätt som första nod
    if (!*head) {
        *head = new_node;
        pthread_mutex_unlock(&list_mutex); // Kritisk sektion slut
        return;
    }
    
    // Fall 2: Icke-tom lista - traversera till slutet och lägg till
    Node* current = *head;
    for (; current->next; current = current->next) /* Går till slutet av listan */ ;
    current->next = new_node;
    
    pthread_mutex_unlock(&list_mutex); // Kritisk sektion slut - insättning klar
}

// Lägger till en ny nod efter en given nod
// Trådsäkerhet: Mutex skyddar liststrukturen under modifiering
// Kritisk för att förhindra korruption om flera trådar ändrar samma område
// TODO: Bör använda mem_alloc istället för malloc enligt uppgiftens krav
void list_insert_after(Node* prev_node, uint16_t data) {
    pthread_mutex_lock(&list_mutex); // Kritisk sektion börjar - skyddar strukturmodifiering
    
    // Felhantering: Kontrollera att prev_node inte är NULL
    if (!prev_node) {
        fprintf(stderr, "Error: prev_node is NULL in list_insert_after.\n");
        pthread_mutex_unlock(&list_mutex); // VIKTIGT: Lås upp även vid fel
        return;
    }
    
    // Allokera ny nod
    // TODO: Använd mem_alloc för att integrera med den anpassade minneshanteraren
    Node* new_node = (Node*)malloc(sizeof(Node));
    
    // Felhantering: Kontrollera om allokeringen lyckades
    if (!new_node) {
        fprintf(stderr, "Error: Memory allocation failed in list_insert_after.\n");
        pthread_mutex_unlock(&list_mutex); // VIKTIGT: Lås upp även vid fel
        return;
    }
    
    // Atomisk insättning: Uppdatera pekare för att länka in den nya noden
    new_node->data = data;
    new_node->next = prev_node->next; // Peka på nästa nod
    prev_node->next = new_node;       // Föregående nod pekar nu på den nya
    
    pthread_mutex_unlock(&list_mutex); // Kritisk sektion slut
}

// Lägger till en ny nod före en given nod
// Trådsäkerhet: Mutex skyddar liststrukturen, inklusive huvudpekaren
// Kräver traversering för att hitta föregångaren, skyddas av låset
// TODO: Bör använda mem_alloc istället för malloc enligt uppgiftens krav
void list_insert_before(Node** head, Node* next_node, uint16_t data) {
    pthread_mutex_lock(&list_mutex); // Kritisk sektion börjar - skyddar struktur och traversering
    
    // Felhantering: Kontrollera att next_node inte är NULL
    if (!next_node) {
        fprintf(stderr, "Error: next_node is NULL in list_insert_before.\n");
        pthread_mutex_unlock(&list_mutex); // VIKTIGT: Lås upp även vid fel
        return;
    }
    
    // Allokera ny nod
    // TODO: Använd mem_alloc för att integrera med den anpassade minneshanteraren
    Node* new_node = (Node*)malloc(sizeof(Node));
    
    // Felhantering: Kontrollera om allokeringen lyckades
    if (!new_node) {
        fprintf(stderr, "Error: Memory allocation failed in list_insert_before.\n");
        pthread_mutex_unlock(&list_mutex); // VIKTIGT: Lås upp även vid fel
        return;
    }
    
    new_node->data = data;
    
    // Fall 1: Insättning före huvudet - uppdatera head-pekaren
    if (*head == next_node) {
        new_node->next = *head;
        *head = new_node;
    } else {
        // Fall 2: Insättning mitt i listan - hitta föregående nod
        Node* current;
        for (current = *head; current && current->next != next_node; current = current->next)
            /* Hittar föregångaren */ ;
        
        // Felhantering: Kontrollera att next_node finns i listan
        if (!current) {
            fprintf(stderr, "Error: next_node not found in list_insert_before.\n");
            free(new_node); // Frigör den allokerade noden vid fel
        } else {
            // Atomisk insättning mellan current och next_node
            new_node->next = next_node;
            current->next = new_node;
        }
    }
    
    pthread_mutex_unlock(&list_mutex); // Kritisk sektion slut
}

// Tar bort första noden med angivet värde från listan
// Trådsäkerhet: Mutex skyddar både sökning och borttagning atomiskt
// Kritisk för att förhindra korruption vid samtidiga borttagningar eller insättningar
// TODO: Bör använda mem_free istället för free enligt uppgiftens krav
void list_delete(Node** head, uint16_t data) {
    pthread_mutex_lock(&list_mutex); // Kritisk sektion börjar - endast en skrivare åt gången
    
    Node* current = *head;
    Node* prev = NULL;
    
    // Sök efter noden med matchande data
    while (current && current->data != data) {
        prev = current;
        current = current->next;
    }
    
    // Fall 1: Nod hittades - ta bort den
    if (current) {
        // Atomisk borttagning: Uppdatera pekare för att koppla bort noden
        if (prev) {
            // Borttagning mitt i eller i slutet av listan
            prev->next = current->next;
        } else {
            // Borttagning av huvudet
            *head = current->next;
        }
        
        // Frigör noden
        // TODO: Använd mem_free för att integrera med den anpassade minneshanteraren
        free(current);
    } else {
        // Fall 2: Nod inte hittad - felhantering
        fprintf(stderr, "Error: Node not found in list_delete.\n");
    }
    
    pthread_mutex_unlock(&list_mutex); // Kritisk sektion slut
}

// Söker efter första noden med angivet värde
// Trådsäkerhet: Låser även läsoperationer i denna grovkorniga design
// Förhindrar att läsa samtidigt som listan modifieras, vilket skulle kunna ge felaktiga resultat
// Alternativ: Read-write lock skulle tillåta samtidiga läsningar för bättre prestanda
Node* list_search(Node** head, uint16_t data) {
    pthread_mutex_lock(&list_mutex); // Kritisk sektion börjar - läsare måste också låsa
    
    // Traversera listan och sök efter matchande data
    for (Node* current = *head; current; current = current->next) {
        if (current->data == data) {
            pthread_mutex_unlock(&list_mutex); // Kritisk sektion slut - nod hittad
            // OBS: Returnerad pekare kan bli ogiltig om annan tråd modifierar listan senare
            // Användaren bör hålla låset eller kopiera data om säker access krävs
            return current;
        }
    }
    
    pthread_mutex_unlock(&list_mutex); // Kritisk sektion slut - nod inte hittad
    return NULL;
}

// Visar hela listans innehåll
// Trådsäkerhet: Låser under hela utskriften för konsekvent vy av listan
// Förhindrar att listan ändras mitt under utskrift vilket skulle kunna ge inkonsistent output
// Används främst för felsökning (debugging)
void list_display(Node** head) {
    pthread_mutex_lock(&list_mutex); // Kritisk sektion börjar - behöver konsekvent vy
    
    printf("[");
    Node* current = *head;
    int is_first = 1;
    
    // Traversera och skriv ut varje nod
    while (current) {
        if (!is_first) printf(", ");
        printf("%d", current->data);
        is_first = 0;
        current = current->next;
    }
    printf("]");
    
    pthread_mutex_unlock(&list_mutex); // Kritisk sektion slut
}

// Visar ett delområde av listan mellan två noder
// Trådsäkerhet: Låser under hela utskriften för konsekvent vy
// Förhindrar att noder ändras eller tas bort under visning
// Används främst för felsökning (debugging)
void list_display_range(Node** head, Node* start_node, Node* end_node) {
    pthread_mutex_lock(&list_mutex); // Kritisk sektion börjar - behöver konsekvent vy
    
    // Felhantering: Tom lista
    if (!*head) {
        printf("[]");
        pthread_mutex_unlock(&list_mutex); // VIKTIGT: Lås upp även vid tidig retur
        return;
    }
    
    // Börja från start_node eller från huvudet om start_node är NULL
    Node* current = start_node ? start_node : *head;
    printf("[");
    int first = 1;
    
    // Traversera och skriv ut noder tills end_node nås
    do {
        if (!first) printf(", ");
        printf("%d", current->data);
        first = 0;
        
        if (current == end_node) break; // Sluta vid slutnoden
        current = current->next;
    } while (current);
    
    printf("]");
    pthread_mutex_unlock(&list_mutex); // Kritisk sektion slut
}

// Räknar antalet noder i listan
// Trådsäkerhet: Låser under räkning för att undvika race conditions
// Utan lås kan räkningen bli felaktig om noder läggs till/tas bort samtidigt
int list_count_nodes(Node** head) {
    pthread_mutex_lock(&list_mutex); // Kritisk sektion börjar - förhindrar samtidiga ändringar
    
    int count = 0;
    // Traversera hela listan och räkna noder
    for (Node* current = *head; current; current = current->next) {
        count++;
    }
    
    pthread_mutex_unlock(&list_mutex); // Kritisk sektion slut
    return count;
}

// Frigör alla noder i listan
// Trådsäkerhet: Exklusiv låsning under hela rensningen är kritisk
// Måste säkerställa att ingen annan tråd accessar listan under nedrivning
// Kallas typiskt vid programavslut eller när listan inte längre behövs
// TODO: Bör använda mem_free istället för free enligt uppgiftens krav
void list_cleanup(Node** head) {
    pthread_mutex_lock(&list_mutex); // Kritisk sektion börjar - exklusiv cleanup
    
    // Traversera och frigör varje nod
    while (*head) {
        Node* current = *head;
        *head = current->next; // Flytta head framåt
        
        // Frigör noden
        // TODO: Använd mem_free för att integrera med den anpassade minneshanteraren
        free(current);
    }
    // Efter loopen är *head redan NULL
    
    pthread_mutex_unlock(&list_mutex); // Kritisk sektion slut - cleanup klar
}
