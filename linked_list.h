#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

/**
 * Structure representing a node in the linked list
 */
typedef struct Node {
    uint16_t data;
    struct Node* next;
} Node;

// Global mutex for thread safety
extern pthread_mutex_t list_mutex;

/**
 * Initializes an empty linked list
 */
void list_init(Node** head, size_t size);

/**
 * Inserts a new node at the end of the list
 */
void list_insert(Node** head, uint16_t data);

/**
 * Inserts a new node after a given node
 */
void list_insert_after(Node* prev_node, uint16_t data);

/**
 * Inserts a new node before a given node
 */
void list_insert_before(Node** head, Node* next_node, uint16_t data);

/**
 * Deletes the first node with the given data
 */
void list_delete(Node** head, uint16_t data);

/**
 * Searches for a node with the given data
 */
Node* list_search(Node** head, uint16_t data);

/**
 * Displays all nodes in the list
 */
void list_display(Node** head);

/**
 * Displays nodes in a range of the list
 */
void list_display_range(Node** head, Node* start_node, Node* end_node);

/**
 * Counts the number of nodes in the list
 */
int list_count_nodes(Node** head);

/**
 * Frees all nodes in the list
 */
void list_cleanup(Node** head);

#endif
