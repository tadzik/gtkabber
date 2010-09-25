INCS=$(shell pkg-config --cflags gtk+-2.0 loudmouth-1.0 lua)
LIBS=$(shell pkg-config --libs gtk+-2.0 loudmouth-1.0 lua)

CC?=clang
CFLAGS=-std=c99 -Wall -Wextra -pedantic -g $(INCS)
#CFLAGS=-std=c99 -Wall -Wextra -pedantic $(INCS)
LD=$(CC)
LDFLAGS=$(LIBS)

all: main

ui.o: ui.c ui.h
	$(CC) $(CFLAGS) -c ui.c

main.o: main.c ui.h
	$(CC) $(CFLAGS) -c main.c

main: main.o ui.o
	$(LD) $(LDFLAGS) main.o ui.o -o main
