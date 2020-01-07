#ifndef EXTLS_LIST_H
#define EXTLS_LIST_H

typedef struct elem_s
{
	extls_size_t idx;
	struct elem_s* next;

} elem_t;

#define PREPEND_TO(head, idx) do {\
		elem_t* elem = malloc(sizeof(elem_t));\
		elem->idx = idx;\
		elem->next = head;\
		head = elem;\
	}while(0)


#define APPEND_TO(head,idx) do{\
	elem_t* elem = malloc(sizeof(elem_t));\
	elem->idx = idx;\
	if(head == NULL) head = elem;\
	else{\
		elem_t* it = head;\
		while(it>next != NULL) it = it->next;\
		it->next = elem;\
		elem->next=NULL;\
	}\
	}while(0)

#define INSERT_AFTER_EQUAL(head, idx, val) do{\
	elem_t* elem = malloc(sizeof(elem_t));\
	elem->idx = idx;\
	if(head == NULL) head = elem;\
	else{\
		elem_t* it = head;\
		while(it->idx != val) it = it->next;\
		elem->next = it->next;\
		it->next = elem;\
	}\
	}while(0)

#define INSERT_BEFORE_EQUAL(head, idx, val) do{\
	elem_t* elem = malloc(sizeof(elem_t));\
	elem->idx = idx;\
	if(head == NULL) head = elem;\
	else{\
		elem_t* it = head, *old = NULL;\
		while(it->idx != val) {old = it; it = it->next;}\
		elem->next = it;\
		old->next = elem;\
	}\
	}while(0)

#define SAFE_FREE(elem) do{\
	elem_t* old = elem;\
	elem = elem->next;\
	free(old);\
	}while(0)
#endif
