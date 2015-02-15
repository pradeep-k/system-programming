
/*
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

int ring_buffer_create(ring_buffer_t **prb, unsigned int size)
{
        *prb = (ring_buffer_t*)calloc(1, sizeof(ring_buffer_t));
        (*prb)->head = 0;
        (*prb)->tail = 0;
        (*prb)->count = 0; 
        return 0;
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
                lwt->next = NULL;
                lwt->prev = NULL;
                ring->count++;
                return 0;
        } else if (ring->count == 1) {
                ring->tail = lwt;

                ring->head->next = lwt;
                ring->head->prev = lwt;

                lwt->next = ring->head;
                lwt->prev = ring->head;
                
                ring->count++;
                return 0;
        }
                
        
        lwt_t next = ring->tail->next;
        lwt_t prev = ring->tail->prev;

        lwt->prev = ring->tail;
        lwt->next = next;

        next->prev = lwt;
        prev->next = lwt;

        ring->tail = lwt;
        ring->count++;
        
        return 0;
}

lwt_t pop(ring_buffer_t *ring) 
{
        lwt_t prev = NULL;
        lwt_t next = NULL;
        lwt_t temp = ring->head;
        
        if ( 0 == ring->count ) {
                return 0;
        } else if ( 1 == ring->count) {
                ring->head = NULL;
                ring->tail = NULL;
                ring->count--;
                return temp;
        } else if ( 2 == ring->count) {
                ring->head = ring->tail;
                ring->head->prev = NULL;
                ring->head->next = NULL;
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

int remove_one(ring_buffer_t* ring, lwt_t lwt)
{
        lwt_t prev = NULL;
        lwt_t next = NULL;
        lwt_t temp = ring->head;
       
        if ( 0 == ring->count ) {
                return 0;
        } else if ( 1 == ring->count ) {
                ring->head = NULL;
                ring->tail = NULL;
                ring->count--;
                return 0;
        } else if ( 2 == ring->count) {
                if (lwt == ring->head) {
                        ring->head = ring->tail;
                        ring->tail->prev = NULL;
                        ring->tail->next = NULL;
                } else if (lwt == ring->tail) {
                        ring->tail = ring->head;
                        ring->head->prev = NULL;
                        ring->head->next = NULL;
                } else {
                    assert(0);
                }
                ring->count--;
                return 0;
        } else {
                prev = lwt->prev;
                next = lwt->next;
        
                prev->next = next;
                next->prev = prev;
        }

        if (lwt == ring->head) {
                ring->head = lwt->next;
        } else if (lwt == ring->tail) {
                ring->tail = lwt->prev;
        }
        ring->count--;
        return 0;
}

int ring_size (ring_buffer_t* ring)
{
        return ring->count;
}
