#ifndef _LIST_H_
#define _LIST_H_

/*
  A general purpose linked list.
*/
struct List {
    List *next;
    List *prev;

    int id;
    void *data;
};

typedef struct List List;

void Enqueue(List *list, void *data, int id);
void *Dequeue(List *list);

void *FindById(List *list, int id);

#endif
