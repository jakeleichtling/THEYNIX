#include "List.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

void Increment(void *data) {
    ++*((int *)data);
}

bool ListTestList() {
    List *list = ListNewList();
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
    ListInsertByIdOrder(list, &w, w);
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

    // Test in-order insertion
    unsigned int first = 1;
    unsigned int second = 2;
    unsigned int third = 3;

    ListInsertByIdOrder(list, &second, second);
    ListInsertByIdOrder(list, &first, first);
    ListInsertByIdOrder(list, &third, third);
    unsigned int * first_dequeue = ListDequeue(list);
    unsigned int * second_dequeue = ListDequeue(list);
    unsigned int * third_dequeue = ListDequeue(list);

    assert(*first_dequeue == first);
    assert(*second_dequeue == second);
    assert(*third_dequeue == third);

    // Test Map function
    assert(ListEmpty(list));
    ListAppend(list, &first, first);
    ListAppend(list, &second, second);
    ListMap(list, &Increment);
    first_dequeue = ListDequeue(list);
    second_dequeue = ListDequeue(list);

    assert(*first_dequeue == 2);
    assert(*second_dequeue == 3);

    ListDestroy(list);

    return true;
}

List *ListNewList() {
    List *list = calloc(1, sizeof(List));
    list->sentinel = calloc(1, sizeof(ListNode));
    list->head = list->sentinel;
    return list;
}

void ListDestroy(List *list) {
    assert(ListEmpty(list));
    free(list->sentinel);
    free(list);
}

bool ListEmpty(List *list) {
    assert(list);
    return (list->sentinel == list->head);
}

void ListPush(List *list, void *data, unsigned int id) {
    assert(list);

    ListNode *ln = malloc(sizeof(ListNode));
    list->head->prev = ln;
    ln->next = list->head;
    list->head = ln;
    ln->id = id;
    ln->data = data;
}

void *ListDequeue(List *list) {
    if (ListEmpty(list)) {
        return NULL;
    }

    ListNode *ln = list->head;
    void* data = ln->data;

    list->head = ln->next;
    list->head->prev = list->sentinel;
    free(ln);
    return data;
}

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

void *ListFindFirstLessThanIdAndRemove(List *list, unsigned int id) {
    if (ListEmpty(list)) {
        return NULL;
    }

    ListNode *iter = list->head;

    while (iter != list->sentinel) {
        if (iter->id >= id) {
            return iter->data;
        }
        iter = iter->next;
    }

    return NULL; // No match
}

void *ListFindById(List *list, unsigned int id) {
    ListNode *node = ListFindNodeById(list, id);
    if (node) { // found
        return node->data;
    } else { // not found
        return NULL;
    }
}

void *ListRemoveById(List *list, unsigned int id) {
    ListNode *node = ListFindNodeById(list, id);
    if (node) {
        if (node == list->head) {
            return ListDequeue(list);
        } else {
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

void ListInsertByIdOrder(List *list, void *data, unsigned int id) {
    assert(list);

    // If list is empty or new id is smallest, simply put in front
    if (ListEmpty(list) || list->head->id > id) {
        ListPush(list, data, id);
        return;
    }

    ListNode *current = list->head;
    while (current->next != list->sentinel && id > current->next->id) {
        current = current->next;
    }

    ListNode *ln = calloc(1, sizeof(ListNode));
    ln->next = current->next;
    ln->next->prev = ln;
    current->next = ln;
    ln->prev = current;

    ln->id = id;
    ln->data = data;
}

void ListEnqueue(List *list, void *data, unsigned int id) {
    ListAppend(list, data, id);
}

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
}

void ListMap(List *list, void (*ftn) (void*)) {
    if (ListEmpty(list)) {
        return;
    }
    ListNode *i;
    for (i = list->head; i && i != list->sentinel; i = i->next) {
        if (i->data) {
            (*ftn)(i->data);
        }
    }
}

void *ListPeak(List *list) {
    return list->head->data;
}
