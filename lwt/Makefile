OBJS= main3.o ring.o lwt.o ring_buffer.o
CFLAGS=-O3 -m32 -I. -Wall -Wextra 
#DEFINES=-DTHINK_TIME
BIN=lwt
CC=gcc

%.o:%.c
	$(CC) $(CFLAGS) $(DEFINES) -o $@ -c $<

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(DEFINES) -o $(BIN) $^

clean:
	rm $(BIN) $(OBJS)

