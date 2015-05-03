/* lwt.h	
 * Pradeep, Yang
 */
#ifndef __LWT_H__
#define __LWT_H__


#include <pthread.h>

#include "ring_buffer.h"
#include "list.h"
#include "waitfree_rb.h"

#define LWT_NULL NULL
#define MAX_THD 64


extern const long LWT_NOJOIN;

typedef void* (*lwt_fn_t) (void *);

typedef long lwt_flags_t;

typedef enum{
	RUN=1,
	WAIT,
	READY,
	COMPLETE,
        FREE // Complete and has been joined
}lwt_status_t;

/* data structures */
struct lwt_tcb{		//thread control block;
	void* ip;
	void* sp;
	void* bp;
        void* return_value;
        lwt_fn_t fn;   
        void*  data;
        /*
         * we may keep a lwt_t array or pool to store all lwt with id
         * as its index, and a current queue to store ids for all currently 
         * living lwt
         */
	int id;
        lwt_status_t status;
        lwt_flags_t flags; 

        /*
         * This thread called join. 
         * Unblock it after we are dead. 
         */
        struct lwt_tcb* blocked[MAX_THD];
	int num_blocked;
        struct lwt_tcb* next;
        struct lwt_tcb* prev;
};

typedef struct lwt_tcb* lwt_t;	//a pointer to tcb, use it as pthread_t
typedef struct lwt_tcb tcb;

typedef enum{
	LWT_INFO_NTHD_RUNNABLE=0,
	LWT_INFO_NTHD_BLOCKED,
	LWT_INFO_NTHD_ZOMBIES
}lwt_info_t;



/*
 * lightweight thread APIs.
 */


lwt_t lwt_create(lwt_fn_t fn, void* data, lwt_flags_t flags);

void* lwt_join(lwt_t thd_handle);

void lwt_die(void *ret);

void lwt_yield(lwt_t thd_handle);

lwt_t lwt_current();

int lwt_id(lwt_t thd_handle);

int lwt_info(lwt_info_t t);


/* 
 *  channel_______________________________________________________
 */

struct lwt_channel_group_t;


typedef enum{
	IDLE,
	RCV
}chan_status_t;

struct lwt_channel {

        /*
         * Threads that are blocked.
         * For sync case, all threads will be added in this list
         * For async, only threads that are blocked will be added in this list.
         */
	unsigned int count_sending;
	struct lwt_list_t* sending_thds;

        // Status of the channel. 
        // IDLE means nobody is expecting any data.
        // RCV means that the thread is blocked waiting for some data.
	chan_status_t status;

        /*
         * Size of recieve buffer.
         */
	chan_buf_t *queue;
        
        //Owner of the channel as only one channel is allowed to own/rcv.
	struct lwt_tcb *rcv_thd;
        
        //Rcver thread can put any data here for any purpose.
        void* mark;

        //All the sender thds that are allow to send to this channel.
	unsigned int count_sender;
	struct lwt_list_t* sender_thds;
        
        // The owner group. One channel can be part of one owner only.
        struct lwt_channel_group_t* parent_grp;

        //If the channel is already in onwer group's active list.
        //This means that event is still pending for this channel for the group.
        unsigned int in_grp_active_list;
        
        //this list makes the group member as a doubly linked-list.
        struct list_head list;
};

typedef struct lwt_channel *lwt_chan_t;

typedef void* (*lwt_chan_fn_t)(lwt_chan_t);

typedef struct __kthd_arg {
        lwt_fn_t fn;
        lwt_chan_t c;
} kthd_arg_t;



lwt_chan_t lwt_chan(int sz);

void lwt_chan_deref(lwt_chan_t c);

int lwt_snd(lwt_chan_t c, void *data);

void *lwt_rcv(lwt_chan_t c);

int lwt_snd_chan(lwt_chan_t c, lwt_chan_t sending);

lwt_chan_t lwt_rcv_chan(lwt_chan_t c);

lwt_t lwt_create_chan(lwt_chan_fn_t fn, lwt_chan_t c, lwt_flags_t flags);

int chan_buf_size(lwt_chan_t c);

/*
 * --------Multi-wait---------------------------
 */

/*
 * all the channels associated with it
 */
typedef struct tag_chan_buf_t chan_queue_t;

struct lwt_channel_group_t {
        
        // Channels with data in its buffer.
        chan_queue_t  active_list;
        
        //Thread waiting on this group.
	struct lwt_tcb *rcv_thd;
	
        //group status
        chan_status_t status;

        // All the channels of this group.
        struct list_head list;
};

typedef struct lwt_channel_group_t* lwt_cgrp_t ;


lwt_cgrp_t lwt_cgrp();

int lwt_cgrp_free(lwt_cgrp_t);

int lwt_cgrp_add(lwt_cgrp_t, lwt_chan_t);

int lwt_cgrp_rem(lwt_cgrp_t, lwt_chan_t);

lwt_chan_t lwt_cgrp_wait(lwt_cgrp_t);

void lwt_chan_mark_set(lwt_chan_t, void *);

void* lwt_chan_mark_get(lwt_chan_t);


/*
 * kthd libraray
 */


struct ringqueue_t;

struct ktcb{
	struct ringqueue_t *lwt_pool;
	struct ringqueue_t *lwt_zombie;
	struct ringqueue_t *lwt_blocked;
	struct ringqueue_t *lwt_run;
	lwt_t current_thd;

        //used for inter-kthd communication
        waitfree_rb_t *thd_rb;
        pthread_mutex_t thd_rb_lock;
        pthread_cond_t  thd_rb_cond;
        
};
typedef struct ktcb* ktcb_t;

void lwt_init();

#endif
