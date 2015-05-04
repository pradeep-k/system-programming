/* lwt.c
 * Pradeep, Yang
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include "ring.h"

#define MAX_THD 64
//#define DEFAULT_STACK_SIZE 1048576 //1MB
#define DEFAULT_STACK_SIZE 548576 //1MB

/*
 *  * Taken from man pages example.
 *   */
#define handle_error_en(en, msg) \
            do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)


//data structure for pthread;
pthread_key_t key=-2;

const long LWT_NOJOIN = 0x1;

unsigned int thd_id = 0;
int thd_pool_size = 50;

//ring_buffer_t   *lwt_pool = NULL;
//ring_buffer_t *lwt_zombie = NULL;
//ring_buffer_t *lwt_blocked = NULL;
//ring_buffer_t *lwt_run = NULL;


lwt_t   Queue[MAX_THD];// a queue to store living thread's pointer
int     queue_length=0;



//int runnable_num=1;
//int blocked_num=0;
int zombies_num=0;//global variable for lwt_info

/*
 * Internal functions.
 */

static void __lwt_schedule(void);

static void __lwt_dispatch(lwt_t current, lwt_t next);

static void __lwt_trampoline();

static void *__lwt_stack_get(void);

static void __lwt_stack_return(void *stk);

static int __lwt_snd_interkthd(lwt_chan_t c, void* data);

static void* __idle_lwt(void* arg);

static void __lwt_unblock(lwt_t next);

/*initialize the lwt_tcb for main function*/

void lwt_current_set()
{
	ktcb_t kthd = pthread_getspecific(key);
        kthd->current_thd = kthd->lwt_run->head;
}



/*
 * APIs
 */


void lwt_init()
{
        int error_code = 0;
	//init the pthread of main function
	pthread_key_create(&key, NULL);
	
        ktcb_t kthd = (ktcb_t)malloc(sizeof(struct ktcb));
	memset(kthd, 0, sizeof(*kthd));
        pthread_setspecific(key,kthd);
        
        //How good it would be if we can make a global lwt pool 
        //instead of local to each kthread.
        kthd->lwt_pool      = ring_buffer_create();
        kthd->lwt_zombie    = ring_buffer_create();
        kthd->lwt_run       = ring_buffer_create();
        kthd->lwt_blocked   = ring_buffer_create();
        
        //lwt tcb for kthread.
        lwt_t main_tcb = (lwt_t)malloc(sizeof(tcb));
        memset(main_tcb, 0, sizeof(*main_tcb));
        
        main_tcb->id  = thd_id++;
        main_tcb->owner_kthd = kthd;
	
        push(kthd->lwt_run, main_tcb);
        lwt_current_set();
        main_tcb->status = RUN;

        //We also need to allocate a wait-free ring buffer 
        //to each kthread.
        kthd->thd_rb        = waitfree_rb_create(256);
        
        if (0 != (error_code = pthread_mutex_init(&kthd->thd_rb_lock, 0))) {
                handle_error_en(error_code, "pthread_mutex_init");
        }
        
        if( 0 != (error_code = pthread_cond_init(&kthd->thd_rb_cond, 0))) {
                handle_error_en(error_code, "pthread_cond_init");
        }
        
        kthd->is_sleeping = 0;

        //We also need to create a "idle" lwt thread in each kthd.
        lwt_create(__idle_lwt, NULL, LWT_NOJOIN);
}

void lwt_clean()
{
        //XXX
        return;
}

lwt_t lwt_create(lwt_fn_t fn, void *data, lwt_flags_t flags)
{
	ktcb_t kthd = pthread_getspecific(key);
        lwt_t thd_handle; 

        if (!is_empty(kthd->lwt_pool)) {
                thd_handle = pop(kthd->lwt_pool);
        } 
	else {

                thd_handle = (lwt_t)malloc(sizeof(tcb));
                memset(thd_handle, 0, sizeof(*thd_handle));
                thd_handle->sp = __lwt_stack_get();
                thd_handle->bp = thd_handle->sp;
        	thd_handle->id = thd_id++; //XXX 
        }
        

	/*
         * Other Initialization.
         */
        thd_handle->ip = __lwt_trampoline;
        thd_handle->sp = thd_handle->bp;
        thd_handle->fn = fn;
        thd_handle->data = data;
       	thd_handle->num_blocked = 0;
        thd_handle->flags = flags; 
        thd_handle->owner_kthd = kthd;
        
        /*
         * Mark the status as READY..
         */
        thd_handle->status = READY;
        push(kthd->lwt_run, thd_handle);
//	printf("create thd %d in thd %d\n",thd_handle->id, lwt_current()->id);	
        return thd_handle;
}

lwt_t lwt_current()
{
	// return a pointer to current thread
	ktcb_t kthd = pthread_getspecific(key);
	return kthd->current_thd;
}

int lwt_id(lwt_t thd)
{
	// return the unique id for the thread
	return thd->id;
}

int lwt_info(lwt_info_t t)
{
	
	ktcb_t kthd = pthread_getspecific(key);
	// debuggingn helper
	switch(t){
		case LWT_INFO_NTHD_RUNNABLE:
			return ring_size(kthd->lwt_run);
		case LWT_INFO_NTHD_BLOCKED:
			return ring_size(kthd->lwt_blocked);
		case LWT_INFO_NTHD_ZOMBIES:
			return ring_size(kthd->lwt_zombie);
		default:
			return 0;
	}
}//done

void* lwt_join(lwt_t thd_handle)
{
	ktcb_t kthd = pthread_getspecific(key);
	// wait for a thread
	lwt_t current = lwt_current();
        
        //If the thd_handle has LWT_NOJOIN set.
        if (thd_handle->flags & LWT_NOJOIN ) {
                assert(0);
                return NULL;
        }

//	printf("thd %d join thd %d\n",current->id,thd_handle->id);
	if(thd_handle->status !=COMPLETE && 
           thd_handle->status !=FREE) {
//		printf("thd %d blocked by thd %d\n",current->id,thd_handle->id);
		current->status = WAIT;
		thd_handle->blocked[thd_handle->num_blocked] = current;
		thd_handle->num_blocked++;
		pop(kthd->lwt_run);
		push(kthd->lwt_blocked,current);
		__lwt_schedule();
	}
	if (thd_handle->status ==  COMPLETE) {
//		printf("back to join thd %d\n",current->id);
		remove_one(kthd->lwt_zombie, thd_handle);
		thd_handle->status = FREE;
		push(kthd->lwt_pool, thd_handle);
		current->status = RUN;
		if (ring_size(kthd->lwt_pool) >= thd_pool_size) {
			lwt_t free_node  = pop(kthd->lwt_pool);
			__lwt_stack_return(free_node->sp);
			free(free_node);
		}

	} 
//	printf("thd %d join thd %d succ\n",current->id,thd_handle->id);
        return thd_handle->return_value;
}

// kill current thread
void lwt_die(void *ret)
{
	ktcb_t kthd = pthread_getspecific(key);
        lwt_t current = lwt_current();

        if (current->flags &LWT_NOJOIN) {
                current->status = FREE;
                pop(kthd->lwt_run);
                push(kthd->lwt_pool, current);
                
                //XXX: could be memory leak 
                current->return_value = ret;
        } else {
                current->status = COMPLETE;
                pop(kthd->lwt_run);
                push(kthd->lwt_zombie, current);
                current->return_value = ret;
                int num = current->num_blocked;
                int i = 0;

                for(i=0;i<num;i++){
                        lwt_t blocked = current->blocked[i];
                
                        if (blocked!=LWT_NULL) {
                                blocked->status = READY;
                                remove_one(kthd->lwt_blocked, blocked);
                                push(kthd->lwt_run, blocked);
//		printf("thd %d unblocked by thd %d\n",blocked->id,current->id);
                        }
                }
        }
//	printf("thd %d die\n",current->id);	
        __lwt_schedule();

}

/*
 *  yield current thread to thd or call schedule funtion when thd is NULL
 */
void lwt_yield(lwt_t next)
{
	ktcb_t kthd = pthread_getspecific(key);
	lwt_t current = lwt_current();

	if(next == LWT_NULL) {
		__lwt_schedule();
                return;                
	}
	else if (next != LWT_NULL && next->status == READY) {
		current->status = READY;
		next->status = RUN;
		remove_one(kthd->lwt_run, next);
		ring_move(kthd->lwt_run);
		push(kthd->lwt_run, next);
		ring_back(kthd->lwt_run);
		lwt_current_set();
//		printf("dispatch from %d to %d\n",current->id,next->id);
                __lwt_dispatch(next, current);
                return;
        } else if ( next->status == WAIT) {
		        current->status = READY;
			next->status = READY;
			remove_one(kthd->lwt_blocked, next);
		        ring_move(kthd->lwt_run);
			push(kthd->lwt_run, next);
		        ring_back(kthd->lwt_run);
		        lwt_current_set();
                        __lwt_dispatch(next, current);
            
        }
}

/*
 * Internal functions.
 */

static void __lwt_schedule(void)
{
	ktcb_t kthd = pthread_getspecific(key);
	// scheduling: switch to next thread in the queue.
	lwt_t current = lwt_current();
//	printf("schedule from %d\n",current->id);
	if(current->status == RUN){
		ring_move(kthd->lwt_run);
	        lwt_current_set();
		current->status = READY;
	} else if (current->status == COMPLETE){
		lwt_current_set();
	} else if (current->status == WAIT) {
		lwt_current_set();
        }

	
	lwt_t next = kthd->lwt_run->head;
	if (next == current){
	        //assert(0);
                return;
	}
	next->status = RUN;
//	printf("dispatch from %d to %d\n",current->id,next->id);
	__lwt_dispatch(next, current);
	return;
}


static void __lwt_dispatch(lwt_t next, lwt_t current)
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

static void __lwt_trampoline()
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

static void *__lwt_stack_get(void){
        return (void*)((int)malloc(DEFAULT_STACK_SIZE) + DEFAULT_STACK_SIZE);
}

static void __lwt_stack_return(void *stk){
	/*
         *  recover memory for a lwt's stack for reuse
         */

}


/*
 * channel
 */


lwt_chan_t lwt_chan(int sz)
{
	lwt_t current = lwt_current();
	lwt_chan_t channel = (struct lwt_channel*)malloc(sizeof(struct lwt_channel));
	memset(channel, 0, sizeof(*channel));

	channel->sending_thds = list_create();
	channel->sender_thds  = list_create();
//	printf("create channel %d %d\n", channel,channel->count_sending);
	channel->rcv_thd = current;
        if (0 == sz) {
                channel->queue = chan_buf_create(1); 
        } else {
                channel->queue = chan_buf_create(sz); 
        }
	return channel;
}
 
void lwt_chan_deref(lwt_chan_t c)
{
	//check
/*
	lwt_t current = lwt_current();
	if(c->rcv_thd == current){
		c->rcv_thd = 0;
		printf("chan %p deref the rcv %p\n",c,current);
	}

	if(remove_one_list(c->sender_thds, current)){
		c->count_sender--;
		printf("chan %p deref one sender %p\n",c,current);
	}
//	printf("channel %d , with %d senders, %d sendings\n",c,c->count_sender, c->count_sending);
	if(c->rcv_thd != LWT_NULL){
		if(c->rcv_thd->status != COMPLETE && c->rcv_thd->status != FREE ){
//			printf("channel %d free failed by rcv thd %d\n",c,c->rcv_thd);
			return;
		}
	}*/
//	printf("check rcv done\n");
/*	if(c->count_sender !=0 ){
		int i;
		node_t* node = c->sender_thds->head;
		for(i=0;i< c->count_sender;i++){
			if(node == NULL){
				break;
			}
			lwt_t thd = node->data;
			if(thd!=LWT_NULL){
				if(thd->status != COMPLETE && thd->status != FREE){
	//				printf("channel %d free failed by sender thd %d\n",c,thd);
					return;
				}
			}
			node = node->next;
		}
	}
*/
/*XXX	if(c->count_sender != 0){
		return;
	}
//	printf("check sender done\n");
	//free
	free_list(c->sender_thds);
	free_list(c->sending_thds);
	printf("free chan %p succ\n",c);
	free(c);
        */
}

static void __lwt_block(lwt_t next)
{
        
	ktcb_t kthd = pthread_getspecific(key);
	lwt_t current = lwt_current();

        pop(kthd->lwt_run);
        current->status = WAIT;
        push(kthd->lwt_blocked, current);

        if (next == LWT_NULL) {
                __lwt_schedule();
                return;
        }
	
        if ( next->status == WAIT) {
                next->status = READY;
                remove_one(kthd->lwt_blocked, next);
                ring_move(kthd->lwt_run);
                push(kthd->lwt_run, next);
                ring_back(kthd->lwt_run);
                lwt_current_set();
                __lwt_dispatch(next, current);
            
        } else if (next->status == READY) {
		next->status = RUN;
		remove_one(kthd->lwt_run, next);
		ring_move(kthd->lwt_run);
		push(kthd->lwt_run, next);
		ring_back(kthd->lwt_run);
		lwt_current_set();
                __lwt_dispatch(next, current);
                 
        }
}


ktcb_t chan_owner_kthd (lwt_chan_t c) 
{
        return c->rcv_thd->owner_kthd; 
}

int lwt_snd(lwt_chan_t c, void *data)
{
	assert(c != NULL);
        assert (data != NULL);
       
	ktcb_t kthd = pthread_getspecific(key);
        lwt_t current = lwt_current();
        
        /*
         * XXX: We need to check if the channel is local or not
         * If not, we need to push data in inter-kthread-buffer
         */        
        if (chan_owner_kthd(c) != kthd) {
                return __lwt_snd_interkthd(c, data);
        }
	
        //block the sender if queue is full.
        while (is_chan_buf_full(c->queue)) {
		push_list(c->sending_thds, current);
		c->count_sending++;
                if ( RCV == c->status) {
                        __lwt_block(c->rcv_thd);
                } else {//if ( 1 == chan_buf_size(c)) { //synchronization case
                        __lwt_block(LWT_NULL);
                }
	} 
       
        chan_buf_push(c->queue, data);

        if ( c->parent_grp) {
                if (0 == c->in_grp_active_list) {
                        chan_buf_push(&c->parent_grp->active_list, c);
                        c->in_grp_active_list = 1;
                }
        }
        
        if ( 1 == chan_buf_size(c)) {//synchronization case
                push_list(c->sending_thds, current);
	        c->count_sending++;
                if ( c->parent_grp ) {
                        if ( RCV == c->parent_grp->status) {
                            __lwt_block(c->parent_grp->rcv_thd);
                        } else { 
                            __lwt_block(LWT_NULL);
                        }
                } else {
                        if ( RCV == c->status) {
                            __lwt_block(c->rcv_thd);
                        } else { 
                            __lwt_block(LWT_NULL);
                        }
                }
        
        } else if (/*( RCV == c->status) && */( WAIT == c->rcv_thd->status)) {
                remove_one(kthd->lwt_blocked, c->rcv_thd);
                c->rcv_thd->status = READY;
                push(kthd->lwt_run, c->rcv_thd);
        }
        return 0;
}

void *lwt_rcv(lwt_chan_t c)
{
	//ktcb_t kthd = pthread_getspecific(key);
	assert(c != NULL);
        
        lwt_t sender = LWT_NULL;
	lwt_t current = lwt_current();
	
        assert (c->rcv_thd == current);

        if (is_chan_buf_empty(c->queue)) {
                c->status = RCV;
                //If as sender is waiting
                if (0 != c->count_sending) {
                        sender = pop_list(c->sending_thds);
                        c->count_sending--;
                        __lwt_block(sender);
                } else {
                        __lwt_block(LWT_NULL);
                }
        }

	if ( c->count_sending > 0 ) {
                    sender = pop_list(c->sending_thds);
                    c->count_sending--;
                    __lwt_unblock(sender);
        }
        
        c->status = IDLE;
        return chan_buf_pop(c->queue);

}


int lwt_snd_chan(lwt_chan_t c, lwt_chan_t chan)
{
        return lwt_snd(c, chan);
}
 
lwt_chan_t lwt_rcv_chan(lwt_chan_t c)
{
        return (lwt_chan_t) lwt_rcv(c);
}

lwt_t lwt_create_chan(lwt_chan_fn_t fn, lwt_chan_t c, lwt_flags_t flags)
{
	ktcb_t kthd = pthread_getspecific(key);
        lwt_t thd_handle; 

        if (!is_empty(kthd->lwt_pool)) {
                thd_handle = pop(kthd->lwt_pool);
        } 
	else {

                thd_handle = (lwt_t)malloc(sizeof(tcb));
                memset(thd_handle, 0, sizeof(*thd_handle));
                thd_handle->sp = __lwt_stack_get();
                thd_handle->bp = thd_handle->sp;
        	thd_handle->id = thd_id++; //XXX 
        }
        thd_handle->ip = __lwt_trampoline;
        thd_handle->sp = thd_handle->bp;
        thd_handle->fn = fn;
        thd_handle->flags = flags; 
        thd_handle->data = (void*)c;
	push_list(c->sender_thds, thd_handle);
	c->count_sender++;
       	thd_handle->num_blocked = 0; 
        thd_handle->status = READY;
        push(kthd->lwt_run, thd_handle);
//	printf("create thd %d in thd %d\n",thd_handle->id, current_thd->id);	
        return thd_handle;
}


int chan_buf_size(lwt_chan_t c) 
{
        return c->queue->size;
}

/*
 * Multi- wait: Grp API
 */
lwt_cgrp_t lwt_cgrp()
{
        lwt_cgrp_t grp = calloc(1, sizeof(struct lwt_channel_group_t));
        if (NULL == grp) {
                return LWT_NULL;
        }
        INIT_LIST_HEAD(&grp->list);
	grp->rcv_thd = lwt_current();
        chan_buf_init(&grp->active_list, 256);        
        return grp;
}

int lwt_cgrp_free(lwt_cgrp_t grp)
{  
        if (0 != grp->active_list.count) {
                return -1; 
        }

        struct list_head * pos, *n;

        list_for_each_safe(pos, n, &grp->list) {
                list_del(pos);
        }
        chan_buf_clean(&grp->active_list);
        free(grp);
        return 0;
}

int lwt_cgrp_add(lwt_cgrp_t grp, lwt_chan_t chan)
{
        if (chan->list.prev == 0 &&
           chan->list.next == 0) { 
                list_add(&chan->list, &grp->list);
                chan->parent_grp = grp;
                return 0;
        }
        return -1;
}

int lwt_cgrp_rem(lwt_cgrp_t grp, lwt_chan_t chan)
{
        //if the channel is still has some event.
        if (!is_chan_buf_empty(chan->queue)) {
                return -1;
        }
        list_del(&chan->list);
        chan->parent_grp = LWT_NULL;
        return 0;
}

lwt_chan_t lwt_cgrp_wait(lwt_cgrp_t grp)
{
        if (0 == grp->active_list.count) {
                grp->status = RCV;
               __lwt_block(LWT_NULL);
        }

        //Some data are in some of channels.
        grp->status = IDLE;
        lwt_chan_t chan = chan_buf_pop(&grp->active_list);
        chan->in_grp_active_list = 0;
        return chan;
}

void lwt_chan_mark_set(lwt_chan_t chan , void * data)
{
        assert(chan->rcv_thd == lwt_current());

        chan->mark = data;
}

void* lwt_chan_mark_get(lwt_chan_t chan)
{
        assert(chan->rcv_thd == lwt_current());
        return chan->mark;
}

void* __pthd_init(void * arg) {
        kthd_arg_t* kthd_arg = (kthd_arg_t*)(arg);
        
        //The lwt associated with kthd should also be non-joinable. 
        lwt_init();
        
        void* ret_value = kthd_arg->fn(kthd_arg->c);

	//Clean the resouces associated with kthd.
        //Nothing to do with return value as it non-joinable thread
        lwt_clean();
}

//Each thread should have its own wait-free ring buffer
int lwt_kthd_create(lwt_fn_t fn, lwt_chan_t c)
{
	pthread_t pthd; 
        kthd_arg_t* kthd_arg = (kthd_arg_t*) malloc(sizeof(kthd_arg_t));

        kthd_arg->fn = fn;
        kthd_arg->c = c;
	if ( 0!= pthread_create(&pthd, NULL, __pthd_init, (void*)kthd_arg)) {
                assert(0);
        }

        /*
         * Detach the pthread. It should clean itself. 
         * Remember we didn't had to worry this for main thread, 
         * as it was cleaned as process die, though not a good thing as
         *  we need to clean pthread resouces on our own now.
         *  The cleaning should be kthd's trampoline. 
         */
        if ( 0 != pthread_detach(pthd)) {
                assert(0);
        }
        return 0;

}

/*
 * Put msg in the right channel
 */
int handle_msg(inter_kthd_msg_t msg) 
{
        assert(msg->type == MSG);

        ktcb_t kthd;
        
        // If channel is synchronous, then send a reply to remote sender
        // to unblock the remote lwt.
        lwt_snd(msg->rcv_chan, msg->data);
       
        if ( 1 == chan_buf_size(msg->rcv_chan)) {//synchronization case
                msg->type = REPLY;
                msg->data = NULL;
                waitfree_rb_push(kthd->thd_rb, msg);
                
                if (kthd->is_sleeping) {
                        pthread_cond_signal(&kthd->thd_rb_cond);
                } 
        } else {
                free(msg);
        }
        return 0;
}

static 
void __lwt_unblock(lwt_t next)
{
        ktcb_t kthd = pthread_getspecific(key);

        next->status = READY;
        remove_one(kthd->lwt_blocked, next);
        push(kthd->lwt_run, next);
        
}

int handle_reply(inter_kthd_msg_t msg) 
{
        assert(msg->type == REPLY);
        __lwt_unblock(msg->sender);
        free(msg);
        return 0;
}

/*
 * If all lwts in this kthread are blocked because all of them were 
 * waiting for a msg from some remote lwt and I (idle lwt as in ideal) 
 * of each kthread doesn't find any msg in inter-thread buffer
 * then I will sleep. When some remote lwt send a msg,
 * they will wake me using conditional variable.
 * XXX: With this idle thread, some scheduler logic could be simplified like
 * in case of empty run-queue
 *
 * The main job of this thd is to get the data from inter-kthd buffer and put it
 * in the reciver thd/channel.
*/

static void* __idle_lwt(void* arg)
{
        ktcb_t kthd = pthread_getspecific(key);
        while (1) {
                if (is_waitfree_rb_empty(kthd->thd_rb)) {
                        /*
                         * If runqueue is empty then sleep.
                         */
                        if (1 == ring_size(kthd->lwt_run)) {
                                kthd->is_sleeping = 0;
                                pthread_mutex_lock(&kthd->thd_rb_lock);
                                pthread_cond_wait(&kthd->thd_rb_cond, &kthd->thd_rb_lock);
                                pthread_mutex_unlock(&kthd->thd_rb_lock);
                                kthd->is_sleeping = 1;
                        } else {
                            lwt_yield(LWT_NULL);
                            continue;
                        }
                }
                //inter kthd rb has some data.
                inter_kthd_msg_t msg = waitfree_rb_pop(kthd->thd_rb);
                switch(msg->type) {
                    case MSG:
                        handle_msg(msg);
                        break;
                    case REPLY:
                        handle_reply(msg);
                        break;
                }
        }
        return NULL;
}

/*
 * If channel is synchronous, block the sending thrd on its kthd.
 * The idle thd will check the msg recived from the remote channel.
 * Remote channel has to send back a msg saying msg is recieved.
 * 
 * If the channel is asynchronous, we don't have any idea about the buffer
 * size of the remote channel, we will never block in this case.
 */
static int 
__lwt_snd_interkthd(lwt_chan_t c, void* data)
{
        ktcb_t kthd = chan_owner_kthd(c);
        inter_kthd_msg_t msg = (inter_kthd_msg_t*)malloc(sizeof(struct inter_kthd_msg)); 
        msg->sender = lwt_current();
        msg->rcv_chan = c;
        msg->type = MSG;
        msg->data = data;
        waitfree_rb_push(kthd->thd_rb, data);

        /*
         * Is the remote thd sleeping because they are expecting some remote msg
         */
        if (kthd->is_sleeping) {
                pthread_cond_signal(&kthd->thd_rb_cond);
        } 
        
        if ( 1 == chan_buf_size(c)) {//synchronization case
                __lwt_block(LWT_NULL);
        }
        return 0;
}
