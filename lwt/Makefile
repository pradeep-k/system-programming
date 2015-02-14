OBJS= main1.o ring.o lwt.o
CFLAGS=-g -m32 -I. -Wall -Wextra 
#DEFINES=-DTHINK_TIME
BIN=lwt
CC=gcc

%.o:%.c
	$(CC) $(CFLAGS) $(DEFINES) -o $@ -c $<

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(DEFINES) -o $(BIN) $^

clean:
	rm $(BIN) $(OBJS)

