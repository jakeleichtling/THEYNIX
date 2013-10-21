#include "List.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

bool ListTestList() {
    List *list = ListNewList();
    assert(ListEmpty(list));

    int x = 5;
    unsigned int x_id = 6;
    ListEnqueue(list, &x, x_id);
    assert(!ListEmpty(list));

    int y = 10;
    unsigned int y_id = 11;
    ListEnqueue(list, &y, y_id);

    // Test Find By Id finds element
    assert(ListFindById(list, x_id));
    // Test FindById returns null if no element
    assert(!ListFindById(list, -1));

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

void ListEnqueue(List *list, void *data, unsigned int id) {
    assert(list);

    ListNode *ln = malloc(sizeof(ListNode));
    list->head->prev = ln;
    ln->next = list->head;
    ln->prev = list->sentinel;
    list->head = ln;
    ln->id = id;
    ln->data = data;
}

void *ListDequeue(List *list) {
    assert(!ListEmpty(list));

    ListNode *ln = list->head;
    void* data = ln->data;

    list->head = ln->next;
    list->head->prev = list->sentinel;
    free(ln);
    return data;
}

void *ListFindById(List *list, unsigned int id) {
    if (ListEmpty(list)) {
        return NULL;
    }

    ListNode *iter = list->head;

    while (iter != list->sentinel) {
        if (iter->id == id) {
            return iter->data;
        }
        iter = iter->next;
    }

    return NULL; // No match
}

void ListInsertByIdOrder(List *list, void *data, unsigned int id) {
    assert(list);

    // If list is empty or new id is smallest, simply put in front
    if (ListEmpty(list) || list->head->id > id) {
        ListEnqueue(list, data, id);
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

void ListAppend(List *list, void *data, unsigned int id) {
    assert(list);
    if (ListEmpty(list)) {
        ListEnqueue(list, data, id);
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
