#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include <stdint.h>


typedef struct ListCDT * List;

List list_create(int (*cmp)(void * elem_a, void * elem_b), uint64_t size_of_elem);
List list_add(List list, void * data);
List list_remove(List list, void * data);
void list_free(List list);
List list_begin_iter(List list);
int list_has_next(List list);
void * list_get_next(List list, void * buffer);
uint64_t list_get_size(List list);

#endif
