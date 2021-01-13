SRCS=$(wildcard *.c)
OBJS=$(patsubst %.c, %.o, $(SRCS))
HEADS=$(wildcard *.h)
CC = gcc
LIBS = -lpthread -ldl -lrt -lm 

LOCAL_PATH = -I/usr/lib -L/usr/local/lib

CFLAGS=-g
LDFLAGS=
TARGET=read_serial
$(TARGET):$(OBJS)
	$(CC) $(OBJS) $(LOCAL_PATH)  -o $@ $(LDFLAGS) $(LIBS)
	$(CC) ../server.c -o ./ajax3.cgi

%.o:%.c
	$(CC)  $(LOCAL_PATH) -c $< -o $@ $(CFLAGS) $(LIBS)

clean:
	rm -rf *.o $(TARGET)
