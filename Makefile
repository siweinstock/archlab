CC = g++
CFLAGS = -Wall -g

all: asm iss

iss: iss.o
	$(CC) $(CFLAGS) -o iss iss.o

iss.o: iss.cpp iss.hh
	$(CC) $(CFLAGS) -c iss.cpp

asm: asm.c
	gcc -Wall asm.c -o asm