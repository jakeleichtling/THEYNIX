#ifndef _LIST_H_
#define _LIST_H_

/*
  A general purpose linked list.
*/
typedef struct {
    List *next;
    List *prev;

    int id;
    void *data;
} List;

void Enqueue(List *list, void *data, int id);
void *Dequeue(List *list);

void *FindById(List *list, int id);

#endif
