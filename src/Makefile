INCS=$(shell pkg-config --cflags gtk+-2.0 loudmouth-1.0 lua)
LIBS=$(shell pkg-config --libs gtk+-2.0 loudmouth-1.0 lua)

CC=clang
CFLAGS=-std=c99 -Wall -Wextra -pedantic -ggdb $(INCS)
COMPILE=$(CC) $(CFLAGS) -c
#CFLAGS=-std=c99 -Wall -Wextra -pedantic $(INCS)
LD=$(CC)
LDFLAGS=$(LIBS)

all: main

gtk_tbim.o: gtk_tbim.h gtk_tbim.c
	$(COMPILE) gtk_tbim.c

mlentry.o: mlentry.h mlentry.c
	$(COMPILE) mlentry.c

ui.o: common.h ui.c ui.h
	$(COMPILE) ui.c

ui_tabs.o: common.h ui_tabs.c ui_tabs.h gtk_tbim.h
	$(COMPILE) ui_tabs.c

main.o: main.c ui.h
	$(COMPILE) main.c

main: main.o ui.o ui_tabs.o gtk_tbim.o mlentry.o
	$(LD) $(LDFLAGS) main.o ui.o ui_tabs.o gtk_tbim.o mlentry.o -o main

clean:
	rm -f main *.o
