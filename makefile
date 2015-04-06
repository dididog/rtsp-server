#TARGET=rtp_server
TARGET=rtsp_server
OBJ=rtp.o event_queue.o rtsp.o rtsp_method.o rtsp_session.o main.o
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

rtsp.o:rtsp.c
	gcc -c $(CFLAGS) $^

rtsp_method.o:rtsp_method.c
	gcc -c $(CFLAGS) $^

rtsp_session.o:rtsp_session.c
	gcc -c $(CFLAGS) $^

main.o:main.c
	gcc -c $(CFLAGS) $^

test_event_queue.o:test_event_queue.c
	gcc -c $(CFLAGS) $^



clean:
	rm -f $(OBJ)
	rm -f $(TARGET)
	rm -f *.gch

