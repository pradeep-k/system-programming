#ifndef __RING_BUFFER_H_
#define __RING_BUFFER_H_


typedef struct tag_chan_buf_t {
        void       **buf;
        unsigned int size;
        int          head;
        int          tail;
        unsigned int count;
} chan_buf_t;

chan_buf_t* chan_buf_create(unsigned int size);
void chan_buf_cleanup(chan_buf_t* ring_buffer);
int chan_buf_push(chan_buf_t* ring_buffer, void* data);
void* chan_buf_pop(chan_buf_t* ring_buffer);
int is_chan_buf_empty(chan_buf_t* ring_buffer);
int is_chan_buf_full(chan_buf_t* ring_buffer);

#endif
