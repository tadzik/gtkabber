PREFIX?=/usr
INSTALLDIR?=$(DESTDIR)$(PREFIX)

all: compile

install: install-core

install-core: compile
	install -d $(INSTALLDIR)/share/gtkabber/icons
	install -d $(INSTALLDIR)/bin
	install -m644 src/icons/* $(INSTALLDIR)/share/gtkabber/icons
	install -D -m755 src/main $(INSTALLDIR)/share/gtkabber/gtkabber
	echo '#!/bin/sh' > $(INSTALLDIR)/bin/gtkabber
	echo "cd $(INSTALLDIR)/share/gtkabber" >> $(INSTALLDIR)/bin/gtkabber
	echo './gtkabber' >> $(INSTALLDIR)/bin/gtkabber
	chmod 755 $(INSTALLDIR)/bin/gtkabber

compile: src
	@+cd src; make all

clean: src
	@+cd src; make clean

run:
	@cd src; make run
