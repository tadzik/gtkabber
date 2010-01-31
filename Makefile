INCS=$(shell pkg-config --cflags gtk+-2.0 loudmouth-1.0)
LIBS=$(shell pkg-config --libs gtk+-2.0 loudmouth-1.0)
WEXTRA=-Wclobbered -Wempty-body -Wignored-qualifiers -Wmissing-field-initializers -Wmissing-parameter-type -Wold-style-declaration -Woverride-init -Wsign-compare -Wtype-limits -Wuninitialized #Wextra without -Wunused-parameter
PREFIX?=/usr/local
INSTALLDIF?=$(DESTDIR)$(PREFIX)

CC=gcc
CFLAGS=-ansi -Wall -Werror -pedantic $(WEXTRA) -g $(INCS)
LD=gcc
LDFLAGS=$(LIBS)

all: main

commands.o: commands.c ui.h
	@echo "Compiling commands.c"
	@$(CC) $(CFLAGS) -c commands.c

main.o: main.c ui.h
	@echo "Compiling main.c"
	@$(CC) $(CFLAGS) -c main.c

ui.o: types.h ui.c
	@echo "Compiling ui.c"
	@$(CC) $(CFLAGS) -c ui.c

ui_roster.o: types.h ui.c ui_roster.c
	@echo "Compiling ui_roster.c"
	@$(CC) $(CFLAGS) -c ui_roster.c

xmpp.o: types.h ui.h xmpp.c commands.h xmpp_roster.h
	@echo "Compiling xmpp.c"
	@$(CC) $(CFLAGS) -c xmpp.c

xmpp_roster.o: types.h ui.h xmpp_roster.c
	@echo "Compiling xmpp_roster.c"
	@$(CC) $(CFLAGS) -c xmpp_roster.c

main: commands.o main.o ui.o ui_roster.o xmpp.o xmpp_roster.o
	@echo "Linking the whole project"
	@$(LD) $(LDFLAGS) commands.o main.o ui.o ui_roster.o \
					xmpp.o xmpp_roster.o -o main

clean:
	@echo cleaning up...
	@find . -name '*.o' -delete
	@find . -name main -delete

# DO NOT DELETE
