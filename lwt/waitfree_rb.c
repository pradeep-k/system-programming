/*
 * Author: Pradeep Kumar
 * Redistribution of this file is permitted under the GNU General
 * Public License v2.
 */
#include <stdlib.h>
#include "waitfree_rb.h"

waitfree_rb_t* waitfree_rb_create()
{
        waitfree_rb_t* rb = (waitfree_rb_t*) calloc(1, sizeof(waitfree_rb_t)); 
        rb->head = 0;
        rb->tail = 0;
        return  rb;
}


void waitfree_rb_cleanup(waitfree_rb_t* rb)
{
        rb->head = 0;
        rb->tail = 0;
        free(rb);
        return;
}

int waitfree_rb_push(waitfree_rb_t* rb, void* data)
{
        int64_t index =__sync_add_and_fetch(&rb->head, 1);
        rb->buf[index % rb_size] = data;
        return 0;
}

void* waitfree_rb_pop(waitfree_rb_t* rb)
{   
        int64_t index = __sync_fetch_and_add(&rb->tail, 1);
        return rb->buf[index % rb_size];
}

int is_waitfree_rb_empty(waitfree_rb_t* rb)
{
        return  (rb->head == rb->tail);
}

/*int is_waitfree_rb_full(waitfree_rb_t* rb)
{
        return 0;
}*/
