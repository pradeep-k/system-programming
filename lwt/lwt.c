/* lwt.c
 * Pradeep, Yang
 */

#include <pthread.h>
#define MAX_THD 64
#define LWT_NULL NULL

lwt_t lwt_pool[MAX_THD];// a thread pool for reuse, or a table to get tcb pointer by its id
int Queue[MAX_THD];// a queue to store living thread
int queue_length=0;

lwt_t current_thd;//global pointer to current executing thread

int runnable_num=1;
int blocked_num=0;
int zombies_num=0;//global variable for lwt_info


lwt_t lwt_create(lwt_fn_t fn, void *data){
	// set stack, add id to queue and update length
}


void* lwt_join(lwt_t thd_handle){
	// wait for a thread
}

void* lwt_die(void *ret){
	// kill current thread
}

void lwt_yield(lwt_t thd_handle){
	// yield current thread and call schedule funtion, dispatch and schedule
}

lwt_t lwt_current(){
	// return a pointer to current thread
	return current_thd;
}//done

int lwt_id(lwt_t thd){
	// return the unique id for the thread
	return thd->id;
}//done

int lwt_info(lwt_info_t t){
	// debugging helper
	switch(t){
		case LWT_INFO_NTHD_RUNNABLE:
			return runnable_num;
		case LWT_INFO_NTHD_BLOCKED:
			return blocked_num;
		case LWT_INFO_NTHD_ZOMBIES:
			return zombies_num;
		case default:
			return 0;
	}
}//done

/*
    * Internal functions.
     */
void __ lwt_schedule(void){
	// scheduling
}

void __lwt_dispatch(lwt_t next, lwt_t current){
	// context switch from current to next
}

void __lwt_trampoline(){
	// ?
}

void *__lwt_stack_get(void){
	// allocate a new stack for a new lwt
}

void *__lwt_stack_return(void *stk){
	// recover memory for a lwt's stack for reuse 
}

