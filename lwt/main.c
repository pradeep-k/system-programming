#include <stdio.h>
#include "lwt.h"

const unsigned int thd_pool_size = 10;

void foo() 
{
        printf("hello foo\n");
}

void foo1()
{
        printf("hello foo1\n");
}
void foo2()
{
        printf("hello foo2\n");
}

void* start2(void* dummy) 
{
        
        printf("lwt2\n");
        
        //lwt_die(0);

        while (1) {
                lwt_yield(LWT_NULL);
                foo2();
        }
        return 0;
}
void* start(void* dummy) 
{
        printf("lwt1\n");
        
        //lwt_die(0);

        while (1) {
                lwt_yield(LWT_NULL);
                foo1();
        }
        return 0;
}

int main()
{
        lwt_t worker_lwt1, worker_lwt2;
        printf("hello\n");
        lwt_init(thd_pool_size);
        worker_lwt1 = lwt_create(start, NULL);
        worker_lwt2 = lwt_create(start2, NULL);
        lwt_yield(worker_lwt2); 
        printf("main\n");
        lwt_yield(LWT_NULL); 

        lwt_yield(worker_lwt2);
        printf("hello1\n");
        lwt_join (worker_lwt1); 
        while(1) {
            lwt_yield(LWT_NULL);
            foo(); 
        }
         

        return 0;
}
