#ifndef _LIST_H_
#define _LIST_H_

#include <stdbool.h>

/*
  A general purpose linked list.
*/
struct ListNode {
    struct ListNode *next;
    struct ListNode *prev;

    struct ListNode *hash_collission_next;

    unsigned int id;
    void *data;
};

typedef struct ListNode ListNode;

struct List {
    ListNode *sentinel;
    ListNode *head;
    ListNode **hash_table;
    int hash_table_size;
};

typedef struct List List;

// Initializes an empty list.
// User must call ListDestroy() to free.
List *ListNewList(int hash_table_size);

// Free internal structure of list.
// The list must be empty first.
void ListDestroy(List *list);

// Return true if the list has no nodes.
bool ListEmpty(List *list);

// Add a new node to the front of the list.
void ListPush(List *list, void *data, unsigned int id);

// Remove and return the first element in the list.
void *ListDequeue(List *list);

// Return first element with the given id.
void *ListFindById(List *list, unsigned int id);

// Remove the first element with the given id
void *ListRemoveById(List *list, unsigned int id);

// Return first element with id less than or equal to the given id
// element is removed
void *ListFindFirstLessThanIdAndRemove(List *list, unsigned int id);

// Append to end of list
void ListAppend(List *list, void *data, unsigned int id);

// Same as append
void ListEnqueue(List *list, void *data, unsigned int id);

// Apply the given function to each item in the list. The function is passed
// the (void*) data.
void ListMap(List *list, void (*ftn) (void*));

// Returns the head element but does not remove!
void *ListPeak(List *list);

#endif
