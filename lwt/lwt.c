/* lwt.c
 * Pradeep, Yang
 */

#ifndef __LWT_H__
#define __LWT_H__
#include <pthread.h>

#define LWT_NULL NULL
int runnable_num=1;
int blocked_num=0;
int zombies_num=0;//global variable for lwt_info


lwt_t lwt_create(lwt_fn_t fn, void *data){
	//
}


void* lwt_join(lwt_t thd_handle){
	// wait for a thread
}

void* lwt_die(void *ret){
	// kill current thread
}

void lwt_yield(lwt_t thd_handle){
	// yield current thread and call schedule funtion
}

lwt_t lwt_current(){
	// return a pointer to current thread
}

int lwt_id(lwt_t thd){
	// return the unique id for the thread
	return thd->id;
}//done

int lwt_info(lwt_info_t t){
	// debugging helper
	switch(t){
		case LWT INFO NTHD RUNNABLE:
			return runnable_num;
		case LWT INFO NTHD BLOCKED:
			return blocked_num;
		case LWT INFO NTHD ZOMBIES:
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

