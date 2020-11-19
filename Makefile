# Makefile for sha-match

CC= gcc
CFLAGS= -I.

all: smatch sfind sconvert scheck

smatch:	smatch.c SMLib.c
	$(CC) -o bin/smatch smatch.c SMLib.c $(CFLAGS)

sfind:	sfind.c SMLib.c
	$(CC) -o bin/sfind sfind.c SMLib.c $(CFLAGS)

sconvert: sconvert.c SMLib.c
	$(CC) -o bin/sconvert sconvert.c SMLib.c $(CFLAGS)

scheck: scheck.c SMLib.c
	$(CC) -o bin/scheck scheck.c SMLib.c $(CFLAGS)

install:
	cp -fv bin/smatch bin/sfind bin/sconvert bin/scheck sha-match.1 ~/bin/
