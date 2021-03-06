#include <stdio.h>
#include <assert.h>
#include "lwt.h"
#include "ring.h"


//test 1: parent to child
void* child1(lwt_chan_t c) 
{
        int k = 100;
        /* send to parent */
	lwt_snd(c, &k);

        lwt_chan_deref(c);
	return NULL;
}

void* parent1( lwt_chan_t c )
{
	lwt_chan_t  chan = lwt_chan(0);
	lwt_t        thd = lwt_create_chan(child1, chan);
	int            k = 88888;
        int            j = 0;
	
        /* send to main thread */
        lwt_snd(c, &k);
        
        /* Rcv from child thread */
        j = *(int*) lwt_rcv(chan);
	printf("recieved int %d\n", j);
	
        lwt_join(thd);
	lwt_chan_deref(chan);
        lwt_chan_deref(c);
	return NULL;
}

void test1() 
{
	int i = 0;
	printf("======test 1========\n");
        lwt_chan_t chan = lwt_chan(0);
	lwt_t thd = lwt_create_chan(parent1, chan);
	
        lwt_yield(LWT_NULL);
        
        /* rcv from parent1 */
        i = *(int*) lwt_rcv(chan);

	lwt_join(thd);
	lwt_chan_deref(chan);

	printf("run: %d\n",lwt_info(LWT_INFO_NTHD_RUNNABLE));
	printf("zombie: %d\n",lwt_info(LWT_INFO_NTHD_ZOMBIES));
	printf("block: %d\n",lwt_info(LWT_INFO_NTHD_BLOCKED));

}

//test 4: child to parent, send a channel
void* child4(lwt_chan_t c)
{
	int k = 12345;
	printf("child2_1\n");
	lwt_snd(c, (void*)&k);
	printf("child2_2\n");
    
        lwt_chan_deref(c);
        printf("func child2 die\n");
	
        return NULL;
}

void* parent4(lwt_chan_t c)
{
	lwt_chan_t  chan = lwt_chan(0);
	lwt_t        thd = lwt_create_chan(child4, chan);

	printf("parent2_1\n");
        lwt_snd_chan(c, chan);
	printf("parent2_2\n");

        /* 
         * Rcv from main thread and from child2
         */
	int i = *(int*)lwt_rcv(chan);
	printf("recieved int %d\n", i);

        i = *(int*) lwt_rcv(chan);	
	printf("recieved int %d\n", i);

        printf("func parent2 join child2\n");

        lwt_join(thd);
	printf("parent2_3\n");
	lwt_chan_deref(chan);
        lwt_chan_deref(c);

	return NULL;
}

void test4()
{
	printf("====test 2====\n");
        
        lwt_chan_t chan_of_parent2 = 0;
	lwt_chan_t chan = lwt_chan(0);
	lwt_t       thd = lwt_create_chan(parent4, chan);
	int           i = 200;

        lwt_yield(LWT_NULL);

        /*
         * Let's rcv is a channel from parent2.
         */
	printf("test2_1\n");
	chan_of_parent2 = lwt_rcv_chan(chan);
	printf("test2_2\n");

        /*
         * Send something to parent 2.
         */
        lwt_snd(chan_of_parent2, &i);
	printf("test2_3\n");

	lwt_join(thd);
	printf("test2_4\n");
	lwt_chan_deref(chan);
	
        printf("run: %d\n",lwt_info(LWT_INFO_NTHD_RUNNABLE));
	printf("zombie: %d\n",lwt_info(LWT_INFO_NTHD_ZOMBIES));
	printf("block: %d\n",lwt_info(LWT_INFO_NTHD_BLOCKED));

}

//test 2: child to parent, send a channel
void* child2(lwt_chan_t c)
{
	int k = 12345;
	printf("child2_1\n");
	lwt_snd(c, (void*)&k);
	printf("child2_2\n");
    
        lwt_chan_deref(c);
        printf("func child2 die\n");
	
        return NULL;
}

void* parent2(lwt_chan_t c)
{
	lwt_chan_t  chan = lwt_chan(0);
	lwt_chan_t  chan2 = lwt_chan(0);
	lwt_t        thd = lwt_create_chan(child2, chan);

	printf("parent2_1\n");
        lwt_snd_chan(c, chan2);
	printf("parent2_2\n");

        /* 
         * Rcv from main thread and from child2
         */
	int i = *(int*)lwt_rcv(chan2);
	printf("recieved int %d\n", i);

        i = *(int*) lwt_rcv(chan);	
	printf("recieved int %d\n", i);

        printf("func parent2 join child2\n");

        lwt_join(thd);
	printf("parent2_3\n");
	lwt_chan_deref(chan);
        lwt_chan_deref(c);

	return NULL;
}

void test2()
{
	printf("====test 2====\n");
        
        lwt_chan_t chan_of_parent2 = 0;
	lwt_chan_t chan = lwt_chan(0);
	lwt_t       thd = lwt_create_chan(parent2, chan);
	int           i = 200;

        lwt_yield(LWT_NULL);

        /*
         * Let's rcv is a channel from parent2.
         */
	printf("test2_1\n");
	chan_of_parent2 = lwt_rcv_chan(chan);
	printf("test2_2\n");

        /*
         * Send something to parent 2.
         */
        lwt_snd(chan_of_parent2, &i);
	printf("test2_3\n");

	lwt_join(thd);
	printf("test2_4\n");
	lwt_chan_deref(chan);
	
        printf("run: %d\n",lwt_info(LWT_INFO_NTHD_RUNNABLE));
	printf("zombie: %d\n",lwt_info(LWT_INFO_NTHD_ZOMBIES));
	printf("block: %d\n",lwt_info(LWT_INFO_NTHD_BLOCKED));

}
        
void* child3_1(lwt_chan_t c)
{
        lwt_chan_t   chan_to_sibling = 0;
	lwt_chan_t              chan = lwt_chan(0);
        int                        k = 301;
	
        /*
         * Send the channel to parent which will send it to its sibling.
         */
        lwt_snd_chan(c, chan);
	printf("send chan3_1 to parent\n");

        /*
         * Rcv the channel for its sibling through parent
         */
        chan_to_sibling = lwt_rcv_chan(chan);
	printf("rcv chan3_2 from parent\n");

        /*
         * send data to its sibling.
         */
        lwt_snd(chan_to_sibling, &k);
	printf("send to sibling3_2\n");

        /* 
         * Rcv from child2.
         */
	int i = *(int*)lwt_rcv(chan);
        assert(i == 302);
	printf("recieved int %d\n", i);

        i = *(int*) lwt_rcv(chan);	
        assert(i == 300);
	printf("recieved int %d\n", i);

	lwt_chan_deref(chan);
	lwt_chan_deref(chan_to_sibling);
        lwt_chan_deref(c);

	return NULL;
}

void* child3_2(lwt_chan_t c)
{
        lwt_chan_t   chan_to_sibling = 0;
	lwt_chan_t              chan = lwt_chan(0);
        int                        k = 302;
	
        /*
         * Send the channel to parent which will send it to its sibling.
         */
        lwt_snd_chan(c, chan);
	printf("send chan3_2 to parent\n");

        /*
         * Rcv the channel for its sibling through parent
         */
        chan_to_sibling = lwt_rcv_chan(chan);
	printf("rcv chan3_1 from parent\n");

        /*
         * send data to its sibling.
         */
        lwt_snd(chan_to_sibling, &k);
	printf("send to sibling3_1\n");

        /* 
         * Rcv from child2.
         */
	int i = *(int*)lwt_rcv(chan);
        assert(i == 301);
	printf("recieved int %d\n", i);

        i = *(int*) lwt_rcv(chan);	
        assert(i == 300);
	printf("recieved int %d\n", i);

	lwt_chan_deref(chan);
	lwt_chan_deref(chan_to_sibling);
        lwt_chan_deref(c);

	return NULL;
}


void test3()
{
	printf("====test 3 ====\n");
        
        lwt_chan_t chan_of_parent2 = 0;

	lwt_chan_t chan = lwt_chan(0);
	
        lwt_t       thd1 = lwt_create_chan(child3_1, chan);
	lwt_t       thd2 = lwt_create_chan(child3_2, chan);
	
        int           i = 300;

        lwt_yield(LWT_NULL);

        /*
         * Let's rcv is a channel from parent2.
         */
	lwt_chan_t chan_to_child1 = lwt_rcv_chan(chan);
	printf("recv chan_to_child1\n");
        lwt_chan_t chan_to_child2 = lwt_rcv_chan(chan);
	printf("recv chan_to_child2\n");

        /*
         * Send the channel to respective siblings.
         */
        lwt_snd(chan_to_child1, chan_to_child2);
	printf("send  chan to child1\n");
        lwt_snd(chan_to_child2, chan_to_child1);
	printf("send chan to child2\n");
        /*
         * Send something to children as spring break gift.
         * 300 USD is good enough for kids :)
         */
        lwt_snd(chan_to_child1, &i);
	printf("send to child1\n");
        lwt_snd(chan_to_child2, &i);
	printf("send to child2\n");

	lwt_join(thd1);
        printf("func test2 join parent2\n");
        lwt_join(thd2);
	lwt_chan_deref(chan);
	lwt_chan_deref(chan_to_child1);
	lwt_chan_deref(chan_to_child2);
        
}

int main()
{
        lwt_init();
        
        test1(); 
	test2();
	test4();
        //test3();
        return 0;
}
