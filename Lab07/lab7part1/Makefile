# udpCode makefile
# written by Hugh Smith - Feb 2021

CC = gcc
CFLAGS = -g -Wall


SRC = networks.c  gethostbyname.c safeUtil.c
OBJS = networks.o gethostbyname.o safeUtil.o

#uncomment next two lines if your using sendtoErr() library
LIBS += libcpe464.2.21.a -lstdc++ -ldl
CFLAGS += -D__LIBCPE464_

all:  rcopy server

rcopy: udpClient.c $(OBJS) 
	$(CC) $(CFLAGS) -o rcopy udpClient.c pdu_create.c $(OBJS) $(LIBS)

server: udpServer.c $(OBJS) 
	$(CC) $(CFLAGS) -o server udpServer.c pdu_create.c  $(OBJS) $(LIBS)

%.o: %.c *.h 
	gcc -c $(CFLAGS) $< -o $@ 

cleano:
	rm -f *.o

clean:
	rm -f udpServer udpClient *.o

