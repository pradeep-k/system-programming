#ifndef __RING_BUFFER_H_
#define __RING_BUFFER_H_

#include "lwt.h"

typedef struct tag_ring_buffer_t {
        lwt_t   *buf;
        unsigned int size;
        int     head;
        int     tail;
        int     count;
} ring_buffer_t;

int ring_buffer_create(ring_buffer_t** ring_buffer, unsigned int size);
int push(ring_buffer_t* ring_buffer, lwt_t fd);
lwt_t pop(ring_buffer_t* ring_buffer);
int is_empty(ring_buffer_t* ring_buffer);
int is_full(ring_buffer_t* ring_buffer);

#endif
