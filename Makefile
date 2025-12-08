# ELKS-first Makefile
# For ELKS compilation:   make ELKS=1
# For modern Unix:        make

CC = cc
CFLAGS = -O2 -Iinclude

ifdef ELKS
CFLAGS += -D__ELKS__
endif

SRC = src/main.c src/network.c src/commands.c src/ui.c
OUT = elkirc

PREFIX ?= /usr/local
MANDIR ?= $(PREFIX)/share/man
MAN ?= man/elkirc.1
DESTDIR ?=

.PHONY: all install uninstall clean man

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

install: all
	install -d "$(DESTDIR)$(PREFIX)/bin"
	install -m 755 $(OUT) "$(DESTDIR)$(PREFIX)/bin/$(OUT)"
	install -d "$(DESTDIR)$(MANDIR)/man1"
	install -m 644 $(MAN) "$(DESTDIR)$(MANDIR)/man1/"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/bin/$(OUT)"
	rm -f "$(DESTDIR)$(MANDIR)/man1/elkirc.1"

man: $(MAN)
	man -l $(MAN)

clean:
	rm -f $(OUT)