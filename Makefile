CC = g++
CFLAGS = -Wall -g

iss: iss.o
	$(CC) $(CFLAGS) -o iss iss.o

iss.o: iss.cpp iss.hh
	$(CC) $(CFLAGS) -c iss.cpp