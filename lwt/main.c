#include <stdio.h>
#include "lwt.h"

const unsigned int thd_pool_size = 10;

void* start(void* dummy) 
{
        int x = 10;
        int y = 20;
        int z = 0;
        z = x + y;
        printf("z = %d\n", z);
        printf("z = %d\n", z);
        while (1) {
                lwt_yield(LWT_NULL);
        }
        return 0;
}

int main()
{
        printf("hello0\n");
        lwt_init(thd_pool_size);
        lwt_create(start, NULL);
        while (1) {
                lwt_yield(LWT_NULL); 
        }

        return 0;
}
