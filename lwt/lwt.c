/* lwt.c
 * Pradeep, Yang
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>

#include "ring.h"

#define MAX_THD 64
//#define DEFAULT_STACK_SIZE 1048576 //1MB
#define DEFAULT_STACK_SIZE 548576 //1MB


unsigned int thd_id = 0;
unsigned int thd_pool_size = 50;

/*
 * A thread pool for reuse. 
 */
ring_buffer_t   *lwt_pool = NULL;

/*
 * Zombie queue. Move the thread to lwt_pool after join.
 */
ring_buffer_t *lwt_zombie = NULL;

/*
 * blocked threads
 */
ring_buffer_t *lwt_blocked = NULL;

/*
 * Run queue
 */
ring_buffer_t *lwt_runqueue = NULL;


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
        if (NULL != lwt_runqueue) {
            return;
        }
        ring_buffer_create(&lwt_pool );
        ring_buffer_create(&lwt_zombie);
        ring_buffer_create(&lwt_runqueue);
        ring_buffer_create(&lwt_blocked);
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
                thd_handle->bp = thd_handle->sp;
        }
        

	/*
         * Other Initialization.
         */
        thd_handle->ip = __lwt_trampoline;
        thd_handle->sp = thd_handle->bp;
        thd_handle->fn = fn;
        thd_handle->data = data;
        thd_handle->id = thd_id++; //XXX 
        
        /*
         * Mark the status as READY..
         */
        thd_handle->tcb_status = READY;

        /*
         * add it to run queue
         */
        push(lwt_runqueue, thd_handle);
        
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
			return ring_size(lwt_runqueue) + 1;
		case LWT_INFO_NTHD_BLOCKED:
			return ring_size(lwt_blocked);
		case LWT_INFO_NTHD_ZOMBIES:
			return ring_size(lwt_zombie);
		default:
			return 0;
	}
}//done

void* lwt_join(lwt_t thd_handle)
{
	// wait for a thread
        lwt_t current = lwt_current();

        current->tcb_status = WAIT;

        /*
         * Not more than a thread should wait on a thread.
         */
        assert(0 == thd_handle->lwt_blocked);
        
        if (thd_handle->tcb_status ==  COMPLETE) {
                thd_handle->tcb_status = FREE;
                remove_one(lwt_zombie, thd_handle);
                push(lwt_pool, thd_handle);
        } else if (thd_handle->tcb_status == FREE) {
                assert(0); 
        } else {
                thd_handle->lwt_blocked = current;
        }

        /*
         * Now we are blocked, yield to some other thread.
         */
        __lwt_schedule();
        return thd_handle->return_value;
}

// kill current thread
void lwt_die(void *ret)
{
        lwt_t current = lwt_current();
        lwt_t blocked = current->lwt_blocked;

        /*
         * Destroy the stack, tcb etc except the return value.
         *  (How to destroy the stack?? as we are in the stack)
         * Make it as zombie, so that it should not get scheduled. 
         */
        current->return_value = ret;
        
        /*
         * Unblock the thread that was waiting on this thread.
         * If one is wating, you don't need to move to zombie.
         * Move to thread pool directly.
         */
        if (LWT_NULL != blocked) {
                assert(blocked->tcb_status != COMPLETE);
                blocked->tcb_status = READY;
                //XXX remove(lwt_blocked, blocked);
                push(lwt_runqueue, blocked);
                
                current->tcb_status = FREE;
                current->lwt_blocked = LWT_NULL;
                
                if (ring_size(lwt_pool) >= thd_pool_size) {
                        /*
                         * Free the node
                         */
                        lwt_t free_node  = pop(lwt_pool);
                        __lwt_stack_return(free_node->sp);
                        free(free_node);
                }
                push(lwt_pool, current);

        } else {
                current->tcb_status = COMPLETE;
                push(lwt_zombie, current);
        }
        
        __lwt_schedule();

}

/*
 *  yield current thread to thd or call schedule funtion when thd is NULL
 */
void lwt_yield(lwt_t next)
{
	lwt_t current = lwt_current();

        if (next != LWT_NULL) {
		current->tcb_status = READY;
                push(lwt_runqueue, current); 
		
                next->tcb_status = RUN;
                remove_one(lwt_runqueue, next);
                lwt_current_set(next);
                
                __lwt_dispatch(next, current);
                return;
        } else {
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

	lwt_t next = pop(lwt_runqueue);
        lwt_t current = lwt_current();
        

        /*
         * Just one thread case.
         * Continue running the same
         * XXX:what if main thread called die and it is the only thread.
         */
        if (LWT_NULL == next) {
            /*
             * See if someone was waiting on this thread.
             */
            lwt_t blocked = current->lwt_blocked;
            if (LWT_NULL != blocked) {
                if (blocked->tcb_status == WAIT) {
                        if (current->tcb_status == COMPLETE || 
                            current->tcb_status == FREE) {
                                blocked->tcb_status = READY; 
                                //remove(lwt_blocked, blocked);
                                current->lwt_blocked = LWT_NULL;
                                next = blocked;
                        }
                } else if (blocked->tcb_status == COMPLETE) {
                        remove_one(lwt_zombie, blocked);
                        push(lwt_pool, blocked);
                        current->lwt_blocked = LWT_NULL;
                        assert(0); 
                }
            }
            if (LWT_NULL == next) {
                return;
            }
        }


        assert(COMPLETE !=  next->tcb_status);

        if (current->tcb_status == RUN){
                current->tcb_status = READY;
                push(lwt_runqueue, current);
        } else if (current->tcb_status == WAIT) {
               //XXX push(lwt_blocked, current);
        }

        lwt_current_set(next);
        next->tcb_status = RUN;

	__lwt_dispatch(next, current);
        
}


/*void __lwt_dispatch(lwt_t next, lwt_t current)
{
	// context switch from current to next
       __asm__ __volatile__ (
               "pusha\n\t"
               "movl %%esp,%0\n\t"
               "call get_eip\n\t"// source: stack overflow.
               "get_eip:\n\t"
               "popl %%eax\n\t"
               "addl $12, %%eax\n\t" // exactly 122bytes of instructions are there between this and ret
               "movl %%eax,%1\n\t"
               "movl %2,%%esp\n\t"
               "movl %3,%%ebx\n\t"
               "jmp *%%ebx\n\t"
               "popa\n\t"
               :"=m"(current->sp),"=m"(current->ip)
               :"r"(next->sp),"r"(next->ip)
               :"eax","ebx"
       );
       
}*/

void __lwt_dispatch(lwt_t next, lwt_t current)
{
	// context switch from current to next
       __asm__ __volatile__ (
               "pusha\n\t"
               "movl %%esp,%0\n\t"
               "movl $1f,%1\n\t"
               "movl %2,%%esp\n\t"
               "movl %3,%%ebx\n\t"
               "jmp *%%ebx\n\t"
               "1: popa\n\t"
               :"=m"(current->sp),"=m"(current->ip)
               :"r"(next->sp),"r"(next->ip)
               :"eax","ebx"
       );
       
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
        return (void*)((int)malloc(DEFAULT_STACK_SIZE) + DEFAULT_STACK_SIZE);
}

void __lwt_stack_return(void *stk){
	/*
         *  recover memory for a lwt's stack for reuse
         */
        free(stk);

}

