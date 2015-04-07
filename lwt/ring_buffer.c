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

chan_buf_t* chan_buf_create(unsigned int size)
{
        chan_buf_t* prb = (chan_buf_t*)malloc(sizeof (chan_buf_t));
        prb->buf = (void**)calloc(size, sizeof(void*));
        prb->head = 0;
        prb->tail = 0;
        prb->count = 0; 
        prb->size =  size;

        return prb;
}

void chan_buf_init(chan_buf_t* prb ,unsigned int size)
{
        //chan_buf_t* prb = (chan_buf_t*)malloc(sizeof (chan_buf_t));
        prb->buf = (void**)calloc(size, sizeof(void*));
        prb->head = 0;
        prb->tail = 0;
        prb->count = 0; 
        prb->size =  size;

        return;
}

void chan_buf_clean(chan_buf_t* rb)
{
        rb->head = 0;
        rb->tail = 0;
        rb->count = 0; 
        free(rb->buf);
}

void chan_buf_cleanup(chan_buf_t* rb) 
{
        rb->head = 0;
        rb->tail = 0;
        rb->count = 0; 
        free(rb->buf);
        free(rb);
}

int chan_buf_push(chan_buf_t *ring_buffer, void* data) 
{
        if (ring_buffer->count < ring_buffer->size) {
                ring_buffer->head = (ring_buffer->head + 1) % ring_buffer->size;
                ring_buffer->buf[ring_buffer->head] = data;
                ring_buffer->count += 1; 
                return 0;
        }
        return -1;
}

void* chan_buf_pop(chan_buf_t *ring_buffer) 
{
        if (0 == ring_buffer->count ) {
                return 0;
        }
        
        ring_buffer->tail = (ring_buffer->tail + 1) % ring_buffer->size;
        ring_buffer->count -= 1;
        return ring_buffer->buf[ring_buffer->tail];
}

int is_chan_buf_empty(chan_buf_t *ring_buffer) 
{
        return (0 == ring_buffer->count);
}

int is_chan_buf_full(chan_buf_t *ring_buffer) 
{
        return (ring_buffer->size == ring_buffer->count);
}


