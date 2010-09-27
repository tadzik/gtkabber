INCS=$(shell pkg-config --cflags gtk+-2.0 loudmouth-1.0 lua)
LIBS=$(shell pkg-config --libs gtk+-2.0 loudmouth-1.0 lua)

CC=clang
CFLAGS=-std=c99 -Wall -Wextra -pedantic -g $(INCS)
#CFLAGS=-std=c99 -Wall -Wextra -pedantic $(INCS)
LD=$(CC)
LDFLAGS=$(LIBS)

all: main

ui.o: common.h ui.c ui.h
	$(CC) $(CFLAGS) -c ui.c

ui_tabs.o: common.h ui_tabs.c ui_tabs.h
	$(CC) $(CFLAGS) -c ui_tabs.c

main.o: main.c ui.h
	$(CC) $(CFLAGS) -c main.c

main: main.o ui.o ui_tabs.o
	$(LD) $(LDFLAGS) main.o ui.o ui_tabs.o -o main

clean:
	rm -f main *.o
