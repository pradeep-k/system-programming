/* lwt.c
 * Pradeep, Yang
 */

#include<stdlib.h>
#include <pthread.h>
#include <string.h>

#include "ring_buffer.h"

#define MAX_THD 64
#define DEFAULT_STACK_SIZE 1048576 //1MB

/*
 * A thread pool for reuse. 
 */
ring_buffer_t   *lwt_pool;

/*
 * Zombie queue. Move the thread to lwt_pool after join.
 */
ring_buffer_t *lwt_zombie;

/*
 * Run queue
 */
ring_buffer_t *lwt_runqueue;


lwt_t   Queue[MAX_THD];// a queue to store living thread's pointer
int     queue_length=0;


lwt_t current_thd;//global pointer to current executing thread

int runnable_num=1;
int blocked_num=0;
int zombies_num=0;//global variable for lwt_info

/*initialize the lwt_tcb for main function*/

void lwt_current_set(lwt_t new_thd)
{
        current_thd = new_thd;
}

void lwt_main_init()
{

        lwt_t main_tcb = (lwt_t)malloc(sizeof(tcb));
        memset(main_tcb, 0, sizeof(*main_tcb));
        main_tcb->id=0;
        //main_tcb->queue_index=0;
        lwt_current_set(main_tcb);
        main_tcb->tcb_status = RUN;
        //Queue[0]=&main_tcb;
        //push(lwt_runqueue, main_tcb);
}

/*
 * APIs
 */

void lwt_init(unsigned int thread_pool_size)
{
        ring_buffer_create(&lwt_pool, thread_pool_size);
        ring_buffer_create(&lwt_zombie, thread_pool_size);
        ring_buffer_create(&lwt_runqueue, thread_pool_size);
        lwt_main_init(); 
}

lwt_t lwt_create(lwt_fn_t fn, void *data)
{
        lwt_t thd_handle; 

        if (!is_empty(lwt_pool)) {
                thd_handle = pop(lwt_pool);
        } else {

                thd_handle = (lwt_t)malloc(sizeof(tcb));
                memset(thd_handle, 0, sizeof(*thd_handle));
                thd_handle->sp = __lwt_stack_get();
        }
        

	/*
         * Other Initialization.
         */
        thd_handle->ip = __lwt_trampoline;
        thd_handle->bp = 0;
        thd_handle->fn = fn;
        thd_handle->data = data;
        thd_handle->id = 1; //XXX 
        
        /*
         * Mark the status as READY..
         */
        thd_handle->tcb_status = READY;

        /*
         * add it to run queue
         */
        push(lwt_runqueue, thd_handle);

        runnable_num += 1; 
        
        return thd_handle;
}

lwt_t lwt_current()
{
	// return a pointer to current thread
	return current_thd;
}

int lwt_id(lwt_t thd)
{
	// return the unique id for the thread
	return thd->id;
}

int lwt_info(lwt_info_t t)
{
	// debuggingn helper
	switch(t){
		case LWT_INFO_NTHD_RUNNABLE:
			return runnable_num;
		case LWT_INFO_NTHD_BLOCKED:
			return blocked_num;
		case LWT_INFO_NTHD_ZOMBIES:
			return zombies_num;
		default:
			return 0;
	}
}//done

void* lwt_join(lwt_t thd_handle)
{
	// wait for a thread
        return 0;
}

// kill current thread
void lwt_die(void *ret)
{
        lwt_t thd_handle = lwt_current();

        /*
         * Destroy the stack, tcb etc except the return value.
         *  (How to destroy the stack?? as we are in the stack)
         * Make it as zombie, so that it should not get scheduled. 
         */
        thd_handle->return_value = ret;
        thd_handle->tcb_status = COMPLETE;
        __lwt_schedule();

}

void lwt_yield(lwt_t thd)
{
	/*
         *  yield current thread to thd or call schedule funtion when thd is NULL
         */
	lwt_t current = lwt_current();

	if(thd != LWT_NULL) { //how about it is not ready?
		if(thd->tcb_status == READY) {
			current->tcb_status = READY;
			current_thd = thd;
			thd->tcb_status = RUN;
			__lwt_dispatch(thd, current);
			return;
		} else {
			__lwt_schedule();
		}
		return;
	} else{
		__lwt_schedule();
		return;
	}
}

/*
 * Internal functions.
 */

void __lwt_schedule(void)
{
	// scheduling: switch to next thread in the queue.

	lwt_t next_thd = pop(lwt_runqueue);
        lwt_t current_thd = lwt_current();

        /*
         * Just one thread case.
         * Continue running the same
         * XXX:what if main thread called die and it is the only thread.
         */
        if (LWT_NULL == next_thd) {
            return;
        }

        if (current_thd->tcb_status == COMPLETE) {
                push(lwt_zombie, current_thd);
        }
        else {
            push(lwt_runqueue, current_thd);
        }
       
        lwt_current_set(next_thd);

	__lwt_dispatch(next_thd,current_thd);
        
}


void __lwt_dispatch(lwt_t next, lwt_t current)
{
	// context switch from current to next
       __asm__ __volatile__ (
               "pusha\n\t"
               "movl %%esp,%2\n\t"
               "call get_eip\n\t"// source: stack overflow.
               "get_eip:\n\t"
               "popl %%eax\n\t"
               "addl $13, %%eax\n\t" // exactly 13 bytes of instructions are there between this and ret
               "movl %%eax,%3\n\t"
               "movl %0,%%esp\n\t"
               "movl %1,%%ebx\n\t"
               "jmp *%%ebx\n\t"
               "popa\n\t"
               :"=m"(next->sp),"=m"(next->ip)
               :"r"(current->sp),"r"(current->ip)
               :"eax","ebx"
       );
       
       /*
        * By the time, we have reached here, we are already executing the next thread.
        * So, if the older thread has died, then cleanup the stack and other stuff.
        * XXX: Only in case of no thread pool. 
        */

}

void __lwt_trampoline()
{
        
        /* At a time, only one thread runs, Get the tcb data from the global variable, 
         * which scheduler should have set.
         */
        lwt_t current = lwt_current();
        
        /*
         * Call the fn function. Save its return value so that the thread which joins it
         * can get its value.
         */
        lwt_die(current->fn(current->data));

}

void *__lwt_stack_get(void){
	/* 
         * allocate a new stack for a new lwt
         */
        return (void*)((int)malloc(DEFAULT_STACK_SIZE) + DEFAULT_STACK_SIZE);
}

void __lwt_stack_return(void *stk){
	/*
         *  recover memory for a lwt's stack for reuse
         */
        free(stk);

}

