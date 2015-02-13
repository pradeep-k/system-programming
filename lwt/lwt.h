/* lwt.h	
 * Pradeep, Yang
 */

#ifndef __LWT_H__
#define __LWT_H__

#define LWT_NULL NULL

typedef void* (*lwt_fn_t) (void *);

typedef enum{
	RUN=1,
	WAIT,
	READY,
	COMPLETE
}lwt_status_t;

/* data structures */
struct lwt_tcb{		//thread control block;
	void* ip;
	void* sp;
	void* bp;
        void* return_value;
        lwt_fn_t fn;   
        void* data;
	int id;//we may keep a lwt_t array or pool to store all lwt with id as its index, and a current queue to store ids for all currently living lwt
        lwt_status_t tcb_status; 
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

void lwt_init(unsigned int thread_pool_size);

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

#endif
