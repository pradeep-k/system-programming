#include <stdio.h>
#include "lwt.h"

const unsigned int thd_pool_size = 10;

void foo() 
{
        printf("hello main\n");
       
}

void foo1()
{
        printf("hello lwt\n");
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
        while (1) {
                lwt_yield(LWT_NULL);
                foo();
        }
        return 0;
}

int main()
{
        printf("hello0\n");
        lwt_init(thd_pool_size);
        lwt_create(start, NULL);
        lwt_yield(LWT_NULL); 
        printf("hello1\n");
        lwt_yield(LWT_NULL); 
        printf("hello1\n");

        while(1) {
            lwt_yield(LWT_NULL);
            foo1(); 
        }
         

        return 0;
}
