#include "List.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

/* Internal hash table helper methods */

// Add a newly created node to the hash table
// Inserts in front of list if there is a collision
void ListAddToHashTable(List *list, ListNode *ln) {
    assert(list->hash_table);

    int hash_id = ln->id % list->hash_table_size;
    ListNode *collision = list->hash_table[hash_id];
    
    // If no collision, than this will be null (as it should)
    ln->hash_collission_next = collision;
    list->hash_table[hash_id] = ln;
}

// Find the LN with the given ID in the hash table, but
// leave it in
// Returns null if not found
ListNode *ListFindFromHashTable(List *list, unsigned int id) {
    assert(list->hash_table);

    int hash_id = id % list->hash_table_size;
    ListNode *value = list->hash_table[hash_id];

    // iterate through collision list until the proper
    // one is found
    while (NULL != value && value->id != id) {
        value = value->hash_collission_next;
    }

    return value;
}

// Find the LN with the given id, and then remove from the hash table
// (but don't destroy the node!)
// Returns null if not found
ListNode *ListRemoveFromHashTable(List *list, unsigned int id) {
    assert(list->hash_table);

    int hash_id = id % list->hash_table_size;
    ListNode *value = list->hash_table[hash_id];

    if (NULL == value) { // Nothing in this bucket
        return NULL;
    } else if (id == value->id) {
        // First element in the bucket is what we were looking for,
        // so point bucket to next in list and return
        list->hash_table[hash_id] = value->hash_collission_next;
        return value;
    }

    // Wasn't the first element in collision list, so iterate until we can
    // find it
    while(NULL != value->hash_collission_next) {
        ListNode *next = value->hash_collission_next;
        if (next->id == id) { // found it
            // remove from list by pointing current to next's next
            value->hash_collission_next = next->hash_collission_next;
            return next;
        }
        value = next;
    }

    return NULL;
}

/* Testing methods */

// Use to test map ftn
void Increment(void *data) {
    ++*((int *)data);
}

bool ListTestListWithHash() {
    List *list = ListNewList(10);

    unsigned int ids[100];
    int i;

    for (i = 0; i < 100; i++) {
        ids[i] = i;
    }

    for (i = 0; i < 100; i++) {
        if (i % 2) {
            ListEnqueue(list, &ids[i], ids[i]);
        } else {
            ListPush(list, &ids[i], ids[i]);
        }
    }

    for (i = 0; i < 100; i++) {
        unsigned int *result;
        if (i % 2) {
            result = (unsigned int *) ListRemoveById(list, ids[i]);
        } else {
            result = (unsigned int *) ListFindById(list, ids[i]);
        }
        assert(result);
        assert(*result == ids[i]);
    }

    while(!ListEmpty(list)) {
        ListDequeue(list);
    }

    assert(!ListFindById(list, 5));

    return true;
}

bool ListTestListNoHash() {
    List *list = ListNewList(0);
    assert(ListEmpty(list));

    int x = 5;
    unsigned int x_id = 6;
    ListPush(list, &x, x_id);
    assert(!ListEmpty(list));

    int y = 10;
    unsigned int y_id = 11;
    ListPush(list, &y, y_id);

    // Test Find By Id finds element
    assert(ListFindById(list, x_id));
    // Test FindById returns null if no element
    assert(!ListFindById(list, -1));

    // Test Delete
    int w = 9;
    ListAppend(list, &w, w);
    int* w_delete_result = (int *) ListRemoveById(list, w);
    assert(*w_delete_result == w);
    assert(!ListFindById(list, w));

    // Test FindById returns first element with given id
    int z = 4;
    ListAppend(list, &z, x_id);

    int *x_find = (int *) ListFindById(list, x_id);
    assert(*x_find == x);


    // Test elements are dequeued in order
    // Should be Y X Z (enqueue, enqueue, append)
    int *y_dequeue = (int *) ListDequeue(list);
    assert(*y_dequeue == y);

    int *x_dequeue = (int *) ListDequeue(list);
    assert(*x_dequeue == x);

    int *z_dequeue = (int *) ListDequeue(list);
    assert(*z_dequeue == z);

    assert(ListEmpty(list));

    // Test Map function
    assert(ListEmpty(list));
    int first = 1;
    int second = 2;
    int *first_dequeue;
    int *second_dequeue;
    ListAppend(list, &first, first);
    ListAppend(list, &second, second);
    ListMap(list, &Increment);
    first_dequeue = (int *) ListDequeue(list);
    second_dequeue = (int *)ListDequeue(list);

    assert(*first_dequeue == 2);
    assert(*second_dequeue == 3);

    ListDestroy(list);

    return true;
}

/*
 * uncomment to test
int main(int argc, char **argv) {
    assert(ListTestListNoHash());
    assert(ListTestListWithHash());

    return 0;
}
*/

// Initializes an empty list.
// User must call ListDestroy() to free.
// hash_table_size specifies the size of the hash table,
// where 0 does not create one at all.
List *ListNewList(int hash_table_size) {
    List *list = calloc(1, sizeof(List));
    list->sentinel = calloc(1, sizeof(ListNode));
    list->head = list->sentinel;
    list->hash_table_size = hash_table_size;
    if (hash_table_size > 0) {
        list->hash_table = calloc(hash_table_size, sizeof(List*));
    }
    return list;
}

// Free internal structure of list.
// The list must be empty first.
void ListDestroy(List *list) {
    assert(ListEmpty(list));
    if (list->hash_table_size > 0) {
        free(list->hash_table);
    }
    free(list->sentinel);
    free(list);
}

// Return true if the list has no nodes.
bool ListEmpty(List *list) {
    assert(list);
    return (list->sentinel == list->head);
}

// Return true if the list has no nodes.
void ListPush(List *list, void *data, unsigned int id) {
    assert(list);

    // Allocate new node and insert
    ListNode *ln = calloc(1, sizeof(ListNode));
    list->head->prev = ln;
    ln->next = list->head;
    list->head = ln;
    ln->id = id;

    // point to supplied data
    ln->data = data;
    ln->hash_collission_next = NULL;

    // create add to hash table if we have one
    if (list->hash_table_size) {
        ListAddToHashTable(list, ln);
    }
}

// Remove and return the first element in the list.
// Returns null if list is empty
void *ListDequeue(List *list) {
    if (ListEmpty(list)) {
        return NULL;
    }

    ListNode *ln = list->head;
    void* data = ln->data;

    if (list->hash_table_size) {
        ListRemoveFromHashTable(list, ln->id);
    }

    list->head = ln->next;
    list->head->prev = list->sentinel;
    free(ln);
    return data;
}

// Internal helper function
// Use this if finding by id but no hash table!
ListNode *ListFindNodeById(List *list, unsigned int id) {
    if (ListEmpty(list)) {
        return NULL;
    }

    ListNode *iter = list->head;

    while (iter != list->sentinel) {
        if (iter->id == id) {
            return iter;
        }
        iter = iter->next;
    }

    return NULL; // No match
}

// Return first element with id less than or equal to the given id
// element is removed
void *ListFindFirstLessThanIdAndRemove(List *list, unsigned int id) {
    if (ListEmpty(list)) {
        return NULL;
    }

    ListNode *iter = list->head;

    while (iter != list->sentinel) {
        if (iter->id <= id) {
            void *result = iter->data;
            ListRemoveById(list, iter->id);
            return result;
        }
        iter = iter->next;
    }

    return NULL; // No match
}

// Return first element with the given id.
// returns null if not found
void *ListFindById(List *list, unsigned int id) {
    ListNode *node;
    if (list->hash_table_size) { // using hash table
        node =  ListFindFromHashTable(list, id);
    } else { // no hash table, use iter lookup
        node = ListFindNodeById(list, id);
    }

    if (node) { // found
        return node->data;
    } else { // not found
        return NULL;
    }
}


// Remove the first element with the given id
// returns null if not found
void *ListRemoveById(List *list, unsigned int id) {

    ListNode *node;
    if (list->hash_table_size) { // using hash table
        node = ListRemoveFromHashTable(list, id);
    } else { // revert to normal lookup
        node = ListFindNodeById(list, id);
    }

    if (node) {
        if (node == list->head) {
            return ListDequeue(list);
        } else { // do usual removal stuff
            node->prev->next = node->next;
            node->next->prev = node->prev;
            void *result_data = node->data;
            free(node);
            return result_data;
        }
    } else {
        return NULL;
    }
}

// Same as Append
void ListEnqueue(List *list, void *data, unsigned int id) {
    ListAppend(list, data, id);
}

// Append to end of list
void ListAppend(List *list, void *data, unsigned int id) {
    assert(list);
    if (ListEmpty(list)) {
        ListPush(list, data, id);
        return;
    }

    ListNode *ln = calloc(1, sizeof(ListNode));
    ln->id = id;
    ln->data = data;

    ln->prev = list->sentinel->prev;
    ln->prev->next = ln;
    list->sentinel->prev = ln;
    ln->next = list->sentinel;
    ln->hash_collission_next = NULL;

    // Add to hash if we have it
    if (list->hash_table_size) {
        ListAddToHashTable(list, ln);
    }
}

// Apply the given function to each item in the list. The function is passed
// the (void*) data.
void ListMap(List *list, void (*ftn) (void*)) {
    if (ListEmpty(list)) {
        return;
    }
    ListNode *i;

    // For each node, pass the data to supplied function
    for (i = list->head; i && i != list->sentinel; i = i->next) {
        if (i->data) {
            (*ftn)(i->data);
        }
    }
}

// Returns the head element but does not remove!
void *ListPeak(List *list) {
    return list->head->data;
}
