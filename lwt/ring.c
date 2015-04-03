
/* ring.c
 * Author: Pradeep Kumar
 * Redistribution of this file is permitted under the GNU General
 * Public License v2.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ring.h"

/*
 * Utility functions that helps us maintain the funtionality
 * of a buffer ring.
 * Call create() to create the ring buffer.
 * Use push() and pop() to insert and remove the elements from it.
 */

ring_buffer_t* ring_buffer_create()
{
        ring_buffer_t* prb = (ring_buffer_t*)calloc(1, sizeof(ring_buffer_t));
        prb->head = 0;
        prb->tail = 0;
        prb->count = 0; 
        return prb;
}

void cleanup(ring_buffer_t* rb) 
{
        rb->head = 0;
        rb->tail = 0;
        rb->count = 0; 
        free(rb);
}

int push(ring_buffer_t *ring, lwt_t lwt) 
{  
        if (ring->count == 0) {
                ring->head = lwt;
                ring->tail = lwt;
                lwt->next = lwt;
                lwt->prev = lwt;
                ring->count++;
                return 0;
        } 
        
        lwt_t next = ring->tail->next;

        lwt->prev = ring->tail;
        lwt->next = next;

        next->prev = lwt;
        ring->tail->next = lwt;

        ring->tail = lwt;
        ring->count++;
        
        return 0;
}

lwt_t pop(ring_buffer_t *ring) 
{
        lwt_t prev = NULL;
        lwt_t next = NULL;
        lwt_t temp = ring->head;
        
        if ( temp == NULL ) {
                return 0;
        } else if (ring->count == 1) {
                ring->head = NULL;
                ring->tail = NULL;
                ring->count--;
                return temp;
        } 
        prev = temp->prev;
        next = temp->next;
        
        prev->next = next;
        next->prev = prev;

        ring->head = next;
        ring->count--;

        return temp;
}

int is_empty(ring_buffer_t *ring_buffer) 
{
        return (0 == ring_buffer->count);
}

/*int is_full(ring_buffer_t *ring_buffer) 
{
        return (ring_buffer->size == ring_buffer->count);
}*/

int remove_one(ring_buffer_t* ring, lwt_t lwt)	// a bug: if lwt is not a node in ring
{
        lwt_t prev = NULL;
        lwt_t next = NULL;
       
        if ( 0 == ring->count ) {
                return 0;
        } 
	else if ( 1 == ring->count && ring->head==lwt ) {
                ring->head = NULL;
                ring->tail = NULL;
                ring->count--;
                return 0;
        } 
	else{
                prev = lwt->prev;
                next = lwt->next;
        
                prev->next = next;
                next->prev = prev;
        

		if (lwt == ring->head) {
			ring->head = lwt->next;
		} 
		else if (lwt == ring->tail) {
			ring->tail = lwt->prev;
		}
		ring->count--;
		return 0;
	}
}

int ring_size (ring_buffer_t* ring)
{
        return ring->count;
}

int ring_move (ring_buffer_t* ring)
{
	if(!is_empty(ring)){
		ring->head = ring->head->next;
		ring->tail = ring->tail->next;
	}
	return 0;
}

int ring_back (ring_buffer_t* ring)
{
	if(!is_empty(ring)){
		ring->head = ring->head->prev;
		ring->tail = ring->tail->prev;
	}
	return 0;
}


/* list i*/

list_t* list_create(){
	list_t* list = (list_t*)calloc(1, sizeof(list_t));
        list->head = 0;
        list->tail = 0;
        return list;

}

int push_list(list_t* list, lwt_t thd){
	node_t* lwt = (node_t*)calloc(1, sizeof(node_t));
	lwt->data = thd;
        if (list->head == NULL) {
                list->head = lwt;
                list->tail = lwt;
                lwt->next = 0;
                lwt->prev = 0;
                return 0;
        } 
        

        lwt->prev = list->tail;
        lwt->next = 0;

        list->tail->next = lwt;

        list->tail = lwt;
        return 0;

}

lwt_t pop_list(list_t* list){
        node_t* temp = list->head;
        if ( temp == NULL ) {
                return 0;
        }
        node_t* next = NULL;
	lwt_t data = temp->data;	
	if (list->head == list->tail) {
                list->head = NULL;
                list->tail = NULL;
                return data;
        } 
        next = temp->next;
        
        next->prev = 0;

        list->head = next;
	free(temp);
        return data;

}

int remove_one_list(list_t* list, lwt_t lwt){
        node_t* prev = NULL;
        node_t* next = NULL;
       
        if ( 0 == list->head ) {
                return 0;
        } 
	else{
                node_t* temp = list->head;
		while(temp != 0){
			if(temp->data == lwt){
				break;
			}
			temp = temp->next;
		}
		if(temp == 0){
			return 0;
		}

		prev = temp->prev;
                next = temp->next;
        
                if(prev!=0){prev->next = next;}
                if(next!=0){next->prev = prev;}
        

		if (temp == list->head) {
			list->head = temp->next;
		} 
		else if (temp == list->tail) {
			list->tail = temp->prev;
		}
		free(temp);
		return 1;
	}

}

void free_list(list_t* list){
	node_t * node = list->head;
	node_t * temp;
	while(node != 0){
		temp = node;
		node = temp->next;
		free(temp);
	}
	free(list);
}


/* A ring buffer for multi-threaded programs. A single producer pushes
 * objects into the ring, while a single consumer pops them off for
 * processing. If the ring is full (i.e., the consumer is slower than the
 * producer), then the push operation fails.
*/

struct ring* ring_create(int size)
{
	if (size <= 0)  return NULL;
	
	struct ring *rb=(struct ring*)malloc(sizeof(struct ring));
	rb->push = 0;
	rb->pop = 0;
	rb->size = size;
	rb->count = 0;
	
	rb->buffer = (void**)malloc(size*sizeof(void*));
	if(rb->buffer==NULL){
		return NULL;//no enough memory to allocate, return 0
	}
	return rb;
}

int ring_push(struct ring *rb, void* data)
{ 
	if(rb->count>=rb->size)
		return -1;
	else
	{
		rb->buffer[rb->push] = data;
		rb->count = rb->count + 1;
		rb->push = (rb->push+1)%rb->size;
		return 0;
	}
	
}



void* ring_pop(struct ring *rb)
{	
	void *data;
	if(rb->count <= 0)
		return NULL;
	else{	
	data = rb->buffer[rb->pop];
	rb->count = rb->count - 1;	
	rb->pop = (rb->pop+1)%rb->size;

	return data;
	}
}
