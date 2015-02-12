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
	// create stack, add id to queue and update length
        // Set the sp and ip
        // Add the thread to scheduler.
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
	// debuggingn helper
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
void __lwt_schedule(void){
	// scheduling
}

void __lwt_dispatch(lwt_t current, lwt_t next){
	// context switch from current to next
       __asm__ __volatile__ (
               "pusha\n\t"
               "movl %%esp,%0\n\t"
               "call get_eip\n\t"// source: stack overflow.
               "get_eip:\n\t"
               "popl %%eax\n\t"
               "addl $13, %%eax\n\t" // exactly 13 bytes of instructions are there between this and ret
               "movl %%eax,%1\n\t"
               "movl %2,%%esp\n\t"
               "movl %3,%%ebx\n\t"
               "jmp *%%ebx\n\t"
               "popa\n\t"
               :"=m"(next->sp),"=m"(next->ip)
               :"r"(current->sp),"r"(current->ip)
               :"eax","ebx"
       );
}

//void __lwt_trampoline(lwt_t new_lwt, lwt_fn_t fn, void*data)
void __lwt_trampoline();
{
        
        /* 0. At a time, only one thread runs, Get the tcb data from the global variable, 
         * which scheduler should have set.
         */
        lwt_t current = lwt_current();
        
        /*
         * 1. Call the fn function. Save its return value so that the thread which joins it
         * can get its value.
         */
        lwt_current->return_value = lwt_current->fn(lwt_current->data);
        /*
         * 2. Destroy the stack, tcb etc except the return value.
         *  (How to destroy the stack?? as we are in the stack)
         * 3. Make it as zombie, so that it should not get scheduled. 
         */

}

void *__lwt_stack_get(void){
	// allocate a new stack for a new lwt
}

void *__lwt_stack_return(void *stk){
	// recover memory for a lwt's stack for reuse 
}

