#include "linked_list.h"
#include <stdio.h>    // For printf and fprintf
#include <stdlib.h>   // For malloc and free
#include <pthread.h>

// Global mutex for thread safety
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
extern pthread_mutex_t list_mutex;

/**
 * Initializes an empty linked list
 */
void list_init(Node** head, size_t size) {
    // Step 1: Lock the mutex to ensure thread safety
    pthread_mutex_lock(&list_mutex);

    // Step 2: Set the head pointer to NULL
    *head = NULL;
    (void)size; // Suppress unused parameter warning

    // Step 3: Unlock the mutex
    pthread_mutex_unlock(&list_mutex);
}

/**
 * Inserts a new node at the end of the list
 */
void list_insert(Node** head, uint16_t data) {
    // Step 1: Lock the mutex to ensure thread safety
    pthread_mutex_lock(&list_mutex);

    // Step 2: Create a new node
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (new_node == NULL) {
        fprintf(stderr, "Error: Memory allocation\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }

    // Step 3: Initialize the new node
    new_node->data = data;
    new_node->next = NULL;

    // Step 4: Handle empty list case
    if (*head == NULL) {
        *head = new_node;
    } else {
        // Step 5: Find the end of the list
        Node* current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        // Step 6: Insert at the end
        current->next = new_node;
    }

    // Step 7: Unlock the mutex
    pthread_mutex_unlock(&list_mutex);
}

/**
 * Inserts a new node after a given node
 */
void list_insert_after(Node* prev_node, uint16_t data) {
    // Step 1: Lock the mutex to ensure thread safety
    pthread_mutex_lock(&list_mutex);

    // Step 2: Check if prev_node is NULL
    if (prev_node == NULL) {
        fprintf(stderr, "Error: prev_node is NULL in list_insert_after.\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }

    // Step 3: Create a new node
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (new_node == NULL) {
        fprintf(stderr, "Error: Memory allocation failed in list_insert_after.\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }

    // Step 4: Initialize the new node
    new_node->data = data;
    new_node->next = prev_node->next;

    // Step 5: Insert after prev_node
    prev_node->next = new_node;

    // Step 6: Unlock the mutex
    pthread_mutex_unlock(&list_mutex);
}

/**
 * Inserts a new node before a given node
 */
void list_insert_before(Node** head, Node* next_node, uint16_t data) {
    // Step 1: Lock the mutex to ensure thread safety
    pthread_mutex_lock(&list_mutex);

    // Step 2: Check if next_node is NULL
    if (next_node == NULL) {
        fprintf(stderr, "Error: next_node is NULL in list_insert_before.\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }

    // Step 3: Handle insertion at head of list
    if (*head == next_node) {
        Node* new_node = (Node*)malloc(sizeof(Node));
        if (new_node == NULL) {
            fprintf(stderr, "Error: Memory allocation failed in list_insert_before.\n");
            pthread_mutex_unlock(&list_mutex);
            return;
        }
        new_node->data = data;
        new_node->next = *head;
        *head = new_node;
        pthread_mutex_unlock(&list_mutex);
        return;
    }

    // Step 4: Find the node before next_node
    Node* current = *head;
    while (current != NULL && current->next != next_node) {
        current = current->next;
    }

    // Step 5: Check if next_node was found
    if (current == NULL) {
        fprintf(stderr, "Error: next_node not found in list_insert_before.\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }

    // Step 6: Create and initialize new node
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (new_node == NULL) {
        fprintf(stderr, "Error: Memory allocation failed in list_insert_before.\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }
    new_node->data = data;
    new_node->next = next_node;

    // Step 7: Insert before next_node
    current->next = new_node;

    // Step 8: Unlock the mutex
    pthread_mutex_unlock(&list_mutex);
}

/**
 * Deletes the first node with the given data
 */
void list_delete(Node** head, uint16_t data) {
    // Step 1: Lock the mutex to ensure thread safety
    pthread_mutex_lock(&list_mutex);

    // Step 2: Find the node to delete
    Node* current = *head;
    Node* prev = NULL;
    while (current != NULL && current->data != data) {
        prev = current;
        current = current->next;
    }

    // Step 3: Check if node was found
    if (current == NULL) {
        fprintf(stderr, "Error: Data %u not found in list_delete.\n", data);
        pthread_mutex_unlock(&list_mutex);
        return;
    }

    // Step 4: Handle deletion of head node
    if (prev == NULL) {
        *head = current->next;
    } else {
        // Step 5: Remove the node from the list
        prev->next = current->next;
    }

    // Step 6: Free the node
    free(current);

    // Step 7: Unlock the mutex
    pthread_mutex_unlock(&list_mutex);
}

/**
 * Searches for a node with the given data
 */
Node* list_search(Node** head, uint16_t data) {
    // Step 1: Lock the mutex to ensure thread safety
    pthread_mutex_lock(&list_mutex);

    // Step 2: Search for the node
    Node* current = *head;
    while (current != NULL) {
        if (current->data == data) {
            pthread_mutex_unlock(&list_mutex);
            return current;
        }
        current = current->next;
    }

    // Step 3: Unlock the mutex
    pthread_mutex_unlock(&list_mutex);
    return NULL;
}

/**
 * Displays all nodes in the list
 */
void list_display(Node** head) {
    // Step 1: Lock the mutex to ensure thread safety
    pthread_mutex_lock(&list_mutex);

    // Step 2: Start printing the list
    Node* current = *head;
    printf("[");
    while (current != NULL) {
        printf("%u", current->data);
        if (current->next != NULL) {
            printf(", ");
        }
        current = current->next;
    }
    printf("]");

    // Step 3: Unlock the mutex
    pthread_mutex_unlock(&list_mutex);
}

/**
 * Displays nodes in a range of the list
 */
void list_display_range(Node** head, Node* start_node, Node* end_node) {
    // Step 1: Lock the mutex to ensure thread safety
    pthread_mutex_lock(&list_mutex);

    // Step 2: Handle empty list
    if (*head == NULL) {
        printf("[]");
        pthread_mutex_unlock(&list_mutex);
        return;
    }

    // Step 3: Set start node to head if NULL
    if (start_node == NULL) {
        start_node = *head;
    }

    // Step 4: Start printing the range
    Node* current = start_node;
    printf("[");
    int first = 1;

    while (current != NULL) {
        if (!first) {
            printf(", ");
        }
        printf("%u", current->data);
        if (current == end_node) {
            break;
        }
        current = current->next;
        first = 0;
    }
    printf("]");

    // Step 5: Unlock the mutex
    pthread_mutex_unlock(&list_mutex);
}

/**
 * Counts the number of nodes in the list
 */
int list_count_nodes(Node** head) {
    // Step 1: Lock the mutex to ensure thread safety
    pthread_mutex_lock(&list_mutex);

    // Step 2: Count the nodes
    int count = 0;
    Node* current = *head;
    while (current != NULL) {
        count++;
        current = current->next;
    }

    // Step 3: Unlock the mutex
    pthread_mutex_unlock(&list_mutex);
    return count;
}

/**
 * Frees all nodes in the list
 */
void list_cleanup(Node** head) {
    // Step 1: Lock the mutex to ensure thread safety
    pthread_mutex_lock(&list_mutex);

    // Step 2: Free all nodes
    Node* current = *head;
    while (current != NULL) {
        Node* temp = current;
        current = current->next;
        free(temp);
    }

    // Step 3: Set head to NULL
    *head = NULL;

    // Step 4: Unlock the mutex
    pthread_mutex_unlock(&list_mutex);
}
