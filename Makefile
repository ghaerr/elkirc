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

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)
