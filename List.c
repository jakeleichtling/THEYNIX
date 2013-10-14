#include "List.h"
#include <stdlib.h>
#include <assert.h>

int main(int argc, char ** argv) {
    List *list = ListNewList();
    assert(ListEmpty(list));

    int x = 5;
    int x_id = 6;
    ListEnqueue(list, &x, x_id);
    assert(!ListEmpty(list));

    int y = 10;
    int y_id = 11;
    ListEnqueue(list, &y, y_id);

    // Test Find By Id finds element
    assert(ListFindById(list, x_id));
    // Test FindById returns null if no element
    assert(!ListFindById(list, -1));

    // Test FindById returns first element with given id
    int z = 4;
    ListEnqueue(list, &z, x_id);
    int *z_find = (int *) ListFindById(list, x_id);
    assert(*z_find == z);

    // Test elements are dequeued in order
    int *z_dequeue = (int *) ListDequeue(list);
    assert(*z_dequeue == z);

    int *y_dequeue = (int *) ListDequeue(list);
    assert(*y_dequeue == y);

    int *x_dequeue = (int *) ListDequeue(list);
    assert(*x_dequeue == x);

    assert(ListEmpty(list));
    ListDestroy(list);

    return 0;
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

void ListEnqueue(List *list, void *data, int id) {
    assert(list);

    ListNode *ln = calloc(1, sizeof(ListNode));
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

void *ListFindById(List *list, int id) {
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
