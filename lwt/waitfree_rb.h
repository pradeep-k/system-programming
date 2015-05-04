#ifndef __WAIT_FREE_RB_H__
#define __WAIT_FREE_RB_H__
#define rb_size 1024

typedef struct tag_waitfree_rb_t {
        void       *buf[rb_size];
        unsigned int size;
        volatile int64_t          head;
        volatile int64_t          tail;
} waitfree_rb_t;

waitfree_rb_t* waitfree_rb_create();

void waitfree_rb_cleanup(waitfree_rb_t* rb);

int waitfree_rb_push(waitfree_rb_t* rb, void* data);
void* waitfree_rb_pop(waitfree_rb_t* rb);
int is_waitfree_rb_empty(waitfree_rb_t* rb);
//int is_waitfree_rb_full(waitfree_rb_t* rb);

#endif
