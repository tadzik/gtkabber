INCS=$(shell pkg-config --cflags gtk+-2.0 loudmouth-1.0 lua)
LIBS=$(shell pkg-config --libs gtk+-2.0 loudmouth-1.0 lua)

CC?=gcc
CFLAGS=-ansi -Wall -Wextra -pedantic -g $(INCS)
LD=$(CC)
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

ui.o: types.h ui.h ui.c mlentry.h ui_tabs.h
	@echo "Compiling ui.c"
	@$(CC) $(CFLAGS) -c ui.c

ui_roster.o: config.h types.h ui.h ui_tabs.h ui_roster.c ui_roster.h xmpp.h xmpp_roster.h
	@echo "Compiling ui_roster.c"
	@$(CC) $(CFLAGS) -c ui_roster.c

ui_tabs.o: config.h ui_tabs.h ui_tabs.c types.h mlentry.h
	@echo "Compiling ui_tabs.c"
	@$(CC) $(CFLAGS) -c ui_tabs.c

xmpp.o: types.h ui.h ui_tabs.h xmpp.c config.h xmpp_roster.h
	@echo "Compiling xmpp.c"
	@$(CC) $(CFLAGS) -c xmpp.c

xmpp_roster.o: types.h ui.h xmpp.h xmpp_roster.c
	@echo "Compiling xmpp_roster.c"
	@$(CC) $(CFLAGS) -c xmpp_roster.c

main: config.o main.o ui.o ui_roster.o ui_tabs.o xmpp.o xmpp_roster.o mlentry.o
	@echo "Linking the whole project"
	@$(LD) $(LDFLAGS) config.o main.o ui.o ui_roster.o ui_tabs.o \
				mlentry.o xmpp.o xmpp_roster.o -o main

clean:
	@echo cleaning up...
	@find . -name '*.o' -delete
	@find . -name main -delete

run:
	LM_DEBUG="net" ./main

# DO NOT DELETE
