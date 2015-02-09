#ifndef __LWT_H__
#define __LWT_H__

typedef void* (*lwt_fn_t) (void *);


/*
 * lightweight thread APIs.
 */
lwt_t lwt_create(lwt_fn_t fn, void* data);

void* lwt_join(lwt_t thd_handle);

void* lwt_die(void *ret);

void lwt_yield(lwt_t thd_handle);

lwt_t lwt_current();

int lwt_id(lwt_t thd_handle);

int lwt_info(lwt_info_t t);

/*
 * Internal functions.
 */
void __ lwt_schedule(void);

void __lwt_dispatch(lwt_t next, lwt_t current);

void __lwt_trampoline();

void *__lwt_stack_get(void);

void *__lwt_stack_return(void *stk);

#endif
