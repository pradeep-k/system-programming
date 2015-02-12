/* lwt.c
 * Pradeep, Yang
 */

#include <pthread.h>
#define MAX_THD 64
#define LWT_NULL NULL
#define DEFAULT_STACK_SIZE 1048576 //1MB

lwt_t   lwt_pool[MAX_THD];// a thread pool for reuse, or a table to get tcb pointer by its id
lwt_t   Queue[MAX_THD];// a queue to store living thread's pointer
int     queue_length=0;


lwt_t current_thd;//global pointer to current executing thread

int runnable_num=1;
int blocked_num=0;
int zombies_num=0;//global variable for lwt_info

/*initialize the lwt_tcb for main function*/
struct lwt_tcb main_tcb;
main_tcb.id=0;
main_tcb.queue_index=0;
Queue[0]=&main_tcb;

lwt_t lwt_create(lwt_fn_t fn, void *data)
{
        lwt_t thd_handle = (lwt_t)malloc(sizeof(*lwt_t));
        memset(thd_handle, 0, sizeof(*lwt_t));

	/*
         * create stack and set the sp and ip.
         */
        thd_handle->sp = __lwt_stack_get();
        thd_hanlde->ip = __lwt_trampoline;
        thd_handle->bp = 0;
        thd_handle->fn = fn;
        thd_handle->data = data;
        thd_handle->id = 1; //XXX 

        /*
         * add id to queue and update length
         */
        /*
         * Add the thread to scheduler.
         */
        Queue[thd_handle->id] = thd_handle;
        queue_length += 1; 

        return thd_handle;
}


void* lwt_join(lwt_t thd_handle){
	// wait for a thread
}

void* lwt_die(void *ret){
	// kill current thread
}

void lwt_yield(lwt_t thd)
{
	// yield current thread to thd or call schedule funtion when thd is NULL
	lwt_t current = lwt_current();
	if(thd!=LWT_NULL) {//how about it is not ready?
		if(thd->lwt_status==READY) {
			current->tcb_status=READY;
			current_thd = thd;
			thd->tcb_status=RUN;
			__lwt_dispatch(thd,current);
			return;
		} else {
			__lwt_schedule();
		}
		return;
	}
	else{
		__lwt_schedule();
		return;
	}

}

lwt_t lwt_current(){
	// return a pointer to current thread
	return current_thd;
}

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
	// scheduling: switch to next thread in the queue.
	if(queue_length<2){//situation 1: no more than 1 thread in queue, return directly
		return;
	}
	lwt_t next_thd=LWT_NULL;//situation 2: if there are at least 2 thread in queue, try to set next_thd
	int i=current_thd->queue_index;//get the position of current thread, and find next READY thread.
	i++;
	lwt_status_t temp=Queue[i]->lwt_status;
	while(temp!=READY){
		i++;
		if(i==queue_length){//when reach the last one, back to first
			i=0;
		}
		temp=Queue[i]->lwt_status;
		if(i==current_thd->queue_index){//back to current
			return;
		}
	}
	next_thd=Queue[i];
	if(next_thd==LWT_NULL){
		return;
	}//at this point, we get a READY next thread other than current
	if(current_thd->status==RUN){//What if it is COMPLETE?
		current_thd->lwt_status=READY;
	}
	next_thd->lwt_status=RUN;
	current_thd=next_thd;//remember to update the global variable current_thd
	__lwt_dispatch(next_thd,current_thd);
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
        
        /* At a time, only one thread runs, Get the tcb data from the global variable, 
         * which scheduler should have set.
         */
        lwt_t current = lwt_current();
        
        /*
         * Call the fn function. Save its return value so that the thread which joins it
         * can get its value.
         */
        lwt_current->return_value = lwt_current->fn(lwt_current->data);
        /*
         * Destroy the stack, tcb etc except the return value.
         *  (How to destroy the stack?? as we are in the stack)
         * Make it as zombie, so that it should not get scheduled. 
         */

}

void *__lwt_stack_get(void){
	/* 
         * allocate a new stack for a new lwt
         */
        return malloc(DEFAULT_STACK_SIZE);
}

void *__lwt_stack_return(void *stk){
	/*
         *  recover memory for a lwt's stack for reuse
         */
        free(stk);

}

