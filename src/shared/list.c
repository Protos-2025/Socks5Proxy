#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct Node {
	struct Node *next;
	uint8_t *data;
} Node;

typedef struct ListCDT {
	Node *head;
	Node *iter;
	size_t size;
	int (*cmp)(void *, void *);
	uint64_t sizeOfElem;
} ListCDT;

static inline void copy_element_to_buffer(uint8_t *buffer, uint8_t *data, size_t size) {
	if (buffer == NULL || data == NULL) {
		return;
	}
	memcpy(buffer, data, size);
}

List list_create(int (*cmp)(void *, void *), uint64_t size_of_elem) {
	List list = (List)malloc(sizeof(ListCDT));

	if (!list) {
		return NULL;
	}

	list->head = NULL;
	list->size = 0;
	list->cmp = cmp;
	list->sizeOfElem = size_of_elem;
	return list;
}

List list_add(List list, void *data) {
	Node *prev;
	Node *curr = list->head;

	while (curr != NULL && list->cmp(curr->data, data) < 0) {
		prev = curr;
		curr = curr->next;
	}

	if (curr != NULL && list->cmp(curr->data, data) == 0) {
		// Element already exists, do not add it again
		return list;
	}

	Node *aux = malloc(sizeof(struct Node));

	if (aux == NULL) {
		return NULL;
	}

	aux->data = malloc(list->sizeOfElem);

	if (aux->data == NULL) {
		free(aux);
		return NULL;
	}

	copy_element_to_buffer(aux->data, data, list->sizeOfElem);
	aux->next = curr;

	if (curr == list->head) {
		list->head = aux;
	} else {
		prev->next = aux;
	}

	list->size++;
	return list;
}

List list_remove(List list, void *data) {
	if (list == NULL || list->head == NULL) {
		return NULL;
	}

	Node *tmp = list->head;
	Node *prev = NULL;

	while (tmp != NULL) {
		if (list->cmp(tmp->data, data) == 0) {
			if (prev == NULL) {
				list->head = tmp->next;
			} else {
				prev->next = tmp->next;
			}
			free(tmp->data);
			free(tmp);
			list->size--;
			return list;
		}
		prev = tmp;
		tmp = tmp->next;
	}

	return list;
}

void list_free(List list) {
	if (list == NULL) {
		return;
	}

	Node *tmp = list->head;
	Node *next;

	while (tmp != NULL) {
		next = tmp->next;
		free(tmp->data);
		free(tmp);
		tmp = next;
	}

	free(list);
	return;
}

List list_begin_iter(List list) {
	if (list == NULL) {
		return NULL;
	}
	list->iter = list->head;
	return list;
}

int list_has_next(List list) {
	if (list == NULL) {
		return 0;
	}

	return list->iter != NULL;
}

void *list_get_next(List list, void *buffer) {
	if (list->iter == NULL) {
		return NULL;
	}
	
	void *dataPtr = list->iter->data;
	list->iter = list->iter->next;      
	
	if (buffer != NULL) {
		copy_element_to_buffer(buffer, dataPtr, list->sizeOfElem);
	}
	
	return dataPtr;  
}

uint64_t list_get_size(List list) { return list->size; }

