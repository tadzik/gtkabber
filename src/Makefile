INCS=$(shell pkg-config --cflags gtk+-2.0 loudmouth-1.0 lua)
LIBS=$(shell pkg-config --libs gtk+-2.0 loudmouth-1.0 lua)

CC=gcc
CFLAGS=-ansi -Wall -Wextra -pedantic -O2 -g $(INCS)
LD=gcc
LDFLAGS=$(LIBS)

all: main

config.o: config.c types.h xmpp.h ui.h
	@echo "Compiling config.c"
	@$(CC) $(CFLAGS) -c config.c

main.o: main.c types.h ui.h
	@echo "Compiling main.c"
	@$(CC) $(CFLAGS) -c main.c

mlentry.o: mlentry.c mlentry.h
	@echo "Compiling mlentry.c"
	@$(CC) $(CFLAGS) -c mlentry.c

ui.o: types.h ui.c mlentry.h
	@echo "Compiling ui.c"
	@$(CC) $(CFLAGS) -c ui.c

ui_roster.o: config.h types.h ui.c ui_roster.c xmpp.c xmpp_roster.c
	@echo "Compiling ui_roster.c"
	@$(CC) $(CFLAGS) -c ui_roster.c

xmpp.o: types.h ui.h xmpp.c config.h xmpp_roster.h
	@echo "Compiling xmpp.c"
	@$(CC) $(CFLAGS) -c xmpp.c

xmpp_roster.o: types.h ui.h xmpp.h xmpp_roster.c
	@echo "Compiling xmpp_roster.c"
	@$(CC) $(CFLAGS) -c xmpp_roster.c

main: config.o main.o ui.o ui_roster.o xmpp.o xmpp_roster.o mlentry.o
	@echo "Linking the whole project"
	@$(LD) $(LDFLAGS) config.o main.o ui.o ui_roster.o mlentry.o \
					xmpp.o xmpp_roster.o -o main

clean:
	@echo cleaning up...
	@find . -name '*.o' -delete
	@find . -name main -delete

run:
	LM_DEBUG="net" ./main

# DO NOT DELETE
