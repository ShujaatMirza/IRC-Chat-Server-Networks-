# This makefile does NOT create the sircd for you.
# You'll want to compile debug.o into sircd to get
# the debugging functions.
# We suggest adding three targets:  all, clean, test

CC 	= gcc
CFLAGS	= -Wall -DDEBUG
LD	= gcc

LDFLAGS	=
DEFS	=
LIB	= -lpthread

all:	sircd

sircd: 	sircd.c debug.c rtlib.c irc_proto.c
	$(CC)$(DEFS) $(CFLAGS) -c rtlib.c
	$(CC)$(DEFS) $(CFLAGS) -c debug.c
	$(CC)$(DEFS) $(CFLAGS) -c sircd.c
	$(CC)$(DEFS) $(CFLAGS) -c irc_proto.c
	$(LD) -o $@ $(LDFLAGS) sircd.o irc_proto.o rtlib.o debug.o $(LIB)

clean:	
	rm -f *.o
	rm -f core
	rm -f sircd
