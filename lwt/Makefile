OBJS= main3.o ring.o lwt.o ring_buffer.o waitfree_rb.o
CFLAGS=-g -m32 -I. -Wall -Wextra -pthread 
#DEFINES=-DTHINK_TIME
BIN=lwt
CC=gcc

%.o:%.c
	$(CC) $(CFLAGS) $(DEFINES) -o $@ -c $<

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(DEFINES) -o $(BIN) $^

clean:
	rm $(BIN) $(OBJS)

