/*
 * ring.h
 */
#ifndef __RING_H__
#define __RING_H__

#include "lwt.h"
typedef struct ringqueue_t {
    lwt_t       head;
    lwt_t       tail;
    unsigned int count;
} ring_buffer_t ;

ring_buffer_t* ring_buffer_create();
int push(ring_buffer_t* ring_buffer, lwt_t lwt);
lwt_t pop(ring_buffer_t* ring_buffer);
int remove_one(ring_buffer_t* ring_buffer, lwt_t lwt);
//void add(ring_buffer_t* ring_buffer, lwt_t* lwt);
int is_empty(ring_buffer_t* ring_buffer);
int is_full(ring_buffer_t* ring_buffer);
int ring_size(ring_buffer_t* ring_buffer);
int ring_move(ring_buffer_t* ring_buffer);
int ring_back(ring_buffer_t* ring_buffer);


/* list */
typedef struct lwt_node{
	lwt_t data;
	struct lwt_node *prev;
	struct lwt_node *next;
}node_t;

typedef struct lwt_list_t {
	node_t* head;
	node_t* tail;
//	unsigned int count;
} list_t;

list_t* list_create();
int push_list(list_t* list, lwt_t lwt);
lwt_t pop_list(list_t* list);
int remove_one_list(list_t* list, lwt_t lwt);
//void add(ring_buffer_t* ring_buffer, lwt_t* lwt);
//int is_empty_list(list_t* list);
//int is_full_list(list_t* list);
//int ring_size(ring_buffer_t* ring_buffer);
//int ring_move(ring_buffer_t* ring_buffer);
//int ring_back(ring_buffer_t* ring_buffer);
void free_list(list_t* list);
typedef struct ring
{
	int push;
        int pop;
	int size;
	int count;
	
	void** buffer;	
}ring_t;
struct ring* ring_create(int size);
int ring_push(struct ring *rb, void* data);
void* ring_pop(struct ring *rb);
	

#endif
