#ifndef __RING_H__
#define __RING_H__

#include "lwt.h"

typedef struct ring_t {
    lwt_t       head;
    lwt_t       tail;
    unsigned int count;
} ring_buffer_t ;

int ring_buffer_create(ring_buffer_t** ring_buffer, unsigned int size);
int push(ring_buffer_t* ring_buffer, lwt_t lwt);
lwt_t pop(ring_buffer_t* ring_buffer);
int remove(ring_buffer_t* ring_buffer, lwt_t lwt);
//void add(ring_buffer_t* ring_buffer, lwt_t* lwt);
int is_empty(ring_buffer_t* ring_buffer);
int is_full(ring_buffer_t* ring_buffer);
int ring_size(ring_buffer_t* ring_buffer);

#endif
