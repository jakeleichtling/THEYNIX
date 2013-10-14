#ifndef _LIST_H_
#define _LIST_H_

/*
  A general purpose linked list.
*/
typedef struct List List;
struct List {
    List *next;
    List *prev;

    int id;
    void *data;
};

void Enqueue(List *list, void *data, int id);
void *Dequeue(List *list);

void *FindById(List *list, int id);

#endif
