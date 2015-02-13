OBJS= main.o ring_buffer.o lwt.o
CFLAGS=-g -m32 -I. -Wall -Wextra -lpthread
#DEFINES=-DTHINK_TIME
BIN=lwt
CC=gcc

%.o:%.c
	$(CC) $(CFLAGS) $(DEFINES) -o $@ -c $<

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(DEFINES) -o $(BIN) $^

clean:
	rm $(BIN) $(OBJS)

