# Makefile for CPE464 tcp test code
# written by Hugh Smith - April 2019

CC= gcc
CFLAGS= -g -Wall
LIBS = 


all:   myClient myServer

myClient: myClient.c networks.o gethostbyname.o pollLib.o safeUtil.o recvAndSend.o
	$(CC) $(CFLAGS) -o cclient myClient.c networks.o pollLib.o safeUtil.o recvAndSend.o gethostbyname.o $(LIBS)

myServer: myServer.c networks.o gethostbyname.o pollLib.o safeUtil.o recvAndSend.o handle_table.o
	$(CC) $(CFLAGS) -o server myServer.c networks.o pollLib.o safeUtil.o recvAndSend.o gethostbyname.o handle_table.o $(LIBS)

.c.o:
	gcc -c $(CFLAGS) $< -o $@ $(LIBS)

cleano:
	rm -f *.o

clean:
	rm -f server cclient *.o




