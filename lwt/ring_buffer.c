/*
 * Author: Pradeep Kumar
 * Redistribution of this file is permitted under the GNU General
 * Public License v2.
 */

#include <stdlib.h>
#include <string.h>
#include "ring_buffer.h"

/*
 * Utility functions that helps us maintain the funtionality
 * of a buffer ring.
 * Call create() to create the ring buffer.
 * Use push() and pop() to insert and remove the elements from it.
 */

int ring_buffer_create(ring_buffer_t **prb, unsigned int size)
{
        *prb = (ring_buffer_t*)malloc(sizeof(ring_buffer_t));
        (*prb)->buf = (lwt_t*)malloc(size);
        (*prb)->head = 0;
        (*prb)->tail = 0;
        (*prb)->count = 0; 
        (*prb)->size =  size; 

        return 0;
}

void cleanup(ring_buffer_t* rb) 
{
        rb->head = 0;
        rb->tail = 0;
        rb->count = 0; 
        free(rb);
}

int push(ring_buffer_t *ring_buffer, lwt_t fd) 
{
        if (ring_buffer->count < ring_buffer->size) {
                ring_buffer->head = (ring_buffer->head + 1) % ring_buffer->size;
                ring_buffer->buf[ring_buffer->head] = fd;
                ring_buffer->count += 1; 
                return 0;
        }
        return -1;
}

lwt_t pop(ring_buffer_t *ring_buffer) 
{
        if (0 == ring_buffer->count ) {
                return 0;
        }
        
        ring_buffer->tail = (ring_buffer->tail + 1) % ring_buffer->size;
        ring_buffer->count -= 1;
        return ring_buffer->buf[ring_buffer->tail];
}

int is_empty(ring_buffer_t *ring_buffer) 
{
        return (0 == ring_buffer->count);
}

int is_full(ring_buffer_t *ring_buffer) 
{
        return (ring_buffer->size == ring_buffer->count);
}


