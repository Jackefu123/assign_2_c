#include "linked_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
extern pthread_mutex_t list_mutex;

void list_init(Node** head, size_t size) {
    pthread_mutex_lock(&list_mutex);
    *head = NULL;
    (void)size;
    pthread_mutex_unlock(&list_mutex);
}

void list_insert(Node** head, uint16_t data) {
    pthread_mutex_lock(&list_mutex);
    
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (!new_node) {
        fprintf(stderr, "Error: Memory allocation\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }
    
    new_node->data = data;
    new_node->next = NULL;
    
    if (!*head) {
        *head = new_node;
        pthread_mutex_unlock(&list_mutex);
        return;
    }
    
    Node* current = *head;
    for (; current->next; current = current->next);
    current->next = new_node;
    
    pthread_mutex_unlock(&list_mutex);
}

void list_insert_after(Node* prev_node, uint16_t data) {
    pthread_mutex_lock(&list_mutex);
    
    if (!prev_node) {
        fprintf(stderr, "Error: prev_node is NULL in list_insert_after.\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }
    
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (!new_node) {
        fprintf(stderr, "Error: Memory allocation failed in list_insert_after.\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }
    
    new_node->data = data;
    new_node->next = prev_node->next;
    prev_node->next = new_node;
    
    pthread_mutex_unlock(&list_mutex);
}

void list_insert_before(Node** head, Node* next_node, uint16_t data) {
    pthread_mutex_lock(&list_mutex);
    
    if (!next_node) {
        fprintf(stderr, "Error: next_node is NULL in list_insert_before.\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }
    
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (!new_node) {
        fprintf(stderr, "Error: Memory allocation failed in list_insert_before.\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }
    
    new_node->data = data;
    
    if (*head == next_node) {
        new_node->next = *head;
        *head = new_node;
    } else {
        Node* current;
        for (current = *head; current && current->next != next_node; current = current->next);
        
        if (!current) {
            fprintf(stderr, "Error: next_node not found in list_insert_before.\n");
            free(new_node);
        } else {
            new_node->next = next_node;
            current->next = new_node;
        }
    }
    
    pthread_mutex_unlock(&list_mutex);
}

void list_delete(Node** head, uint16_t data) {
    pthread_mutex_lock(&list_mutex);
    
    Node* current = *head;
    Node* prev = NULL;
    
    while (current && current->data != data) {
        prev = current;
        current = current->next;
    }
    
    if (current) {
        if (prev) {
            prev->next = current->next;
        } else {
            *head = current->next;
        }
        free(current);
    } else {
        fprintf(stderr, "Error: Node not found in list_delete.\n");
    }
    
    pthread_mutex_unlock(&list_mutex);
}

Node* list_search(Node** head, uint16_t data) {
    pthread_mutex_lock(&list_mutex);
    
    for (Node* current = *head; current; current = current->next) {
        if (current->data == data) {
            pthread_mutex_unlock(&list_mutex);
            return current;
        }
    }
    
    pthread_mutex_unlock(&list_mutex);
    return NULL;
}

void list_display(Node** head) {
    pthread_mutex_lock(&list_mutex);
    
    printf("[");
    Node* current = *head;
    int is_first = 1;
    
    while (current) {
        if (!is_first) printf(", ");
        printf("%d", current->data);
        is_first = 0;
        current = current->next;
    }
    printf("]");
    
    pthread_mutex_unlock(&list_mutex);
}

void list_display_range(Node** head, Node* start_node, Node* end_node) {
    pthread_mutex_lock(&list_mutex);
    
    if (!*head) {
        printf("[]");
        pthread_mutex_unlock(&list_mutex);
        return;
    }
    
    Node* current = start_node ? start_node : *head;
    printf("[");
    int first = 1;
    
    do {
        if (!first) printf(", ");
        printf("%d", current->data);
        first = 0;
        
        if (current == end_node) break;
        current = current->next;
    } while (current);
    
    printf("]");
    pthread_mutex_unlock(&list_mutex);
}

int list_count_nodes(Node** head) {
    pthread_mutex_lock(&list_mutex);
    
    int count = 0;
    for (Node* current = *head; current; current = current->next) {
        count++;
    }
    
    pthread_mutex_unlock(&list_mutex);
    return count;
}

void list_cleanup(Node** head) {
    pthread_mutex_lock(&list_mutex);
    
    while (*head) {
        Node* current = *head;
        *head = current->next;
        free(current);
    }
    
    pthread_mutex_unlock(&list_mutex);
}
