/* lwt.h	
 * Pradeep, Yang
 */
#ifndef __LWT_H__
#define __LWT_H__

//#include "ring.h"
#define LWT_NULL NULL
#define MAX_THD 64

typedef void* (*lwt_fn_t) (void *);

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
        void* data;
        /*
         * we may keep a lwt_t array or pool to store all lwt with id
         * as its index, and a current queue to store ids for all currently 
         * living lwt
         */
	int id;
        lwt_status_t status;
        
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

void lwt_init();

lwt_t lwt_create(lwt_fn_t fn, void* data);

void* lwt_join(lwt_t thd_handle);

void lwt_die(void *ret);

void lwt_yield(lwt_t thd_handle);

lwt_t lwt_current();

int lwt_id(lwt_t thd_handle);

int lwt_info(lwt_info_t t);

/*
 * Internal functions.
 */

void __lwt_schedule(void);

void __lwt_dispatch(lwt_t current, lwt_t next);

void __lwt_trampoline();

void *__lwt_stack_get(void);

void __lwt_stack_return(void *stk);

/* 
 *  channel_______________________________________________________
 */

typedef enum{
	IDLE,
	RCV
}chan_status_t;

struct lwt_channel {
	unsigned int count_sending;
	struct lwt_list_t* sending_thds;
	chan_status_t status;
	void *data;

	struct lwt_tcb *rcv_thd;
	unsigned int count_sender;
	struct lwt_list_t* sender_thds;
};

typedef struct lwt_channel *lwt_chan_t;

typedef void* (*lwt_chan_fn_t)(lwt_chan_t);



lwt_chan_t lwt_chan(int sz);

void lwt_chan_deref(lwt_chan_t c);

int lwt_snd(lwt_chan_t c, void *data);

void *lwt_rcv(lwt_chan_t c);

int lwt_snd_chan(lwt_chan_t c, lwt_chan_t sending);

lwt_chan_t lwt_rcv_chan(lwt_chan_t c);

lwt_t lwt_create_chan(lwt_chan_fn_t fn, lwt_chan_t c);



#endif
