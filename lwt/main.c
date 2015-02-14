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
        int x = 10;
        int y = 20;
        int z = 0;
        z = x + y;
        printf("z = %d\n", z);
        
        lwt_yield(LWT_NULL);
        
        printf("z +1 = %d\n", z+1);
        lwt_yield(LWT_NULL);
        printf("z +2 = %d\n", z+2);
        
        //lwt_die(0);

        while (1) {
                lwt_yield(LWT_NULL);
                foo2();
        }
        return 0;
}
void* start(void* dummy) 
{
        int x = 10;
        int y = 20;
        int z = 0;
        z = x + y;
        printf("z = %d\n", z);
        
        lwt_yield(LWT_NULL);
        
        printf("z +1 = %d\n", z+1);
        lwt_yield(LWT_NULL);
        printf("z +2 = %d\n", z+2);
        
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
        printf("hello0\n");
        lwt_init(thd_pool_size);
        worker_lwt1 = lwt_create(start, NULL);
        worker_lwt2 = lwt_create(start2, NULL);
        lwt_yield(worker_lwt2); 
        printf("hello1\n");
        lwt_yield(LWT_NULL); 
        printf("hello1\n");

        lwt_yield(worker_lwt2); 
        while(1) {
            lwt_yield(LWT_NULL);
            foo(); 
        }
         

        return 0;
}
