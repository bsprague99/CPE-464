# # Example makefile for CPE464 program 1
# #
# # 

CC = gcc
CFLAGS = -g -Wall -pedantic -ggdb -Wextra

all: trace

trace: trace.c
	$(CC) $(CFLAGS) -o trace trace.c checksum.c  -lpcap 

trace.c: trace.h
	$(CC) $(CFLAGS) -c trace.c

clean:
	rm -f trace