SRCS=$(wildcard *.c)
OBJS=$(patsubst %.c, %.o, $(SRCS))
HEADS=$(wildcard *.h)
CC = gcc
LIBS = -lpthread -ldl -lrt -lm 

LOCAL_PATH = -I/usr/lib -L/usr/local/lib

CFLAGS=-g
LDFLAGS=
TARGET=ajax3.cgi
$(TARGET):$(OBJS)
	$(CC) $(OBJS) $(LOCAL_PATH)  -o $@ $(LDFLAGS) $(LIBS)

%.o:%.c
	$(CC)  $(LOCAL_PATH) -c $< -o $@ $(CFLAGS) $(LIBS)

clean:
	rm -rf *.o $(TARGET)
