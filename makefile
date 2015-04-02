#TARGET=rtp_server
TARGET=test_event_queue
OBJ=rtp.o event_queue.o
OBJ+=test_event_queue.o
LDFLAGS=-lpthread
CFLAGS=-g -Wall
#CFLAGS+=-D DEBUG

all:$(OBJ)
	gcc -o $(TARGET) $(OBJ) $(LDFLAGS)

rtp_server.o: rtp_server.c rtp.h
	gcc -c $(CFLAGS) $^

rtp.o:rtp.c rtp.h
	gcc -c $(CFLAGS) $^

event_queue.o:event_queue.c
	gcc -c $(CFLAGS) $^

test_event_queue.o:test_event_queue.c
	gcc -c $(CFLAGS) $^



clean:
	rm -f $(OBJ)
	rm -f $(TARGET)
	rm -f *.gch

