INCS=$(shell pkg-config --cflags gtk+-2.0 loudmouth-1.0 lua)
LIBS=$(shell pkg-config --libs gtk+-2.0 loudmouth-1.0 lua)

CC?=gcc
CFLAGS=-std=c99 -Wall -Wextra -pedantic -g $(INCS)
LD=$(CC)
LDFLAGS=$(LIBS)

all: main

config.o: config.c types.h xmpp_pres.h ui.h
	@echo "Compiling config.c"
	@$(CC) $(CFLAGS) -c config.c

main.o: main.c types.h ui.h xmpp_conn.h
	@echo "Compiling main.c"
	@$(CC) $(CFLAGS) -c main.c

mlentry.o: mlentry.c mlentry.h
	@echo "Compiling mlentry.c"
	@$(CC) $(CFLAGS) -c mlentry.c

gtk_tbim.o: gtk_tbim.h gtk_tbim.c
	@echo "Compiling gtk_tbim.c"
	@$(CC) $(CFLAGS) -c gtk_tbim.c

ui.o: types.h ui.h ui.c mlentry.h ui_tabs.h
	@echo "Compiling ui.c"
	@$(CC) $(CFLAGS) -c ui.c

ui_roster.o: config.h types.h ui.h ui_tabs.h ui_roster.c ui_roster.h xmpp_roster.h
	@echo "Compiling ui_roster.c"
	@$(CC) $(CFLAGS) -c ui_roster.c

ui_tabs.o: config.h ui_tabs.h ui_tabs.c types.h mlentry.h gtk_tbim.h
	@echo "Compiling ui_tabs.c"
	@$(CC) $(CFLAGS) -c ui_tabs.c

xmpp_conn.o: types.h ui.h config.h xmpp_conn.h xmpp_roster.h
	@echo "Compiling xmpp_conn.c"
	@$(CC) $(CFLAGS) -c xmpp_conn.c

xmpp_pres.o: types.h ui.h config.h xmpp_pres.h xmpp_roster.h
	@echo "Compiling xmpp_pres.c"
	@$(CC) $(CFLAGS) -c xmpp_pres.c

xmpp_iq.o: types.h ui.h config.h xmpp_iq.h xmpp_roster.h
	@echo "Compiling xmpp_iq.c"
	@$(CC) $(CFLAGS) -c xmpp_iq.c

xmpp_mesg.o: types.h ui.h config.h xmpp_mesg.h xmpp_roster.h
	@echo "Compiling xmpp_mesg.c"
	@$(CC) $(CFLAGS) -c xmpp_mesg.c

xmpp_roster.o: types.h ui.h xmpp_roster.c
	@echo "Compiling xmpp_roster.c"
	@$(CC) $(CFLAGS) -c xmpp_roster.c

main: config.o main.o ui.o ui_roster.o ui_tabs.o xmpp_conn.o xmpp_pres.o xmpp_mesg.o xmpp_iq.o xmpp_roster.o mlentry.o gtk_tbim.o
	@echo "Linking the whole project"
	@$(LD) $(LDFLAGS) config.o main.o ui.o ui_roster.o ui_tabs.o gtk_tbim.o \
				mlentry.o xmpp_conn.o xmpp_pres.o xmpp_mesg.o xmpp_iq.o xmpp_roster.o -o main

clean:
	@echo cleaning up...
	@find . -name '*.o' -delete
	@find . -name main -delete

run:
	LM_DEBUG="net" ./main

# DO NOT DELETE
