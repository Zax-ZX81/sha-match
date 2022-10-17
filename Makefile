# Makefile for sha-match

CC= gcc
CFLAGS= -I.

all: smatch sfind sconvert scheck sdup

smatch:	smatch.c SMLib.c
	$(CC) -o bin/smatch smatch.c SMLib.c $(CFLAGS)

sfind:	sfind.c SMLib.c
	$(CC) -o bin/sfind sfind.c SMLib.c $(CFLAGS)

sconvert: sconvert.c SMLib.c
	$(CC) -o bin/sconvert sconvert.c SMLib.c $(CFLAGS)

scheck: scheck.c SMLib.c
	$(CC) -o bin/scheck scheck.c SMLib.c $(CFLAGS)

sdup: sdup.c SMLib.c
	$(CC) -o bin/sdup sdup.c SMLib.c $(CFLAGS)

ssort: ssort.c SMLib.c
	$(CC) -o bin/ssort ssort.c SMLib.c $(CFLAGS)

install:
	cp -fv bin/smatch bin/sfind bin/sconvert bin/scheck bin/sdup bin/ssort sha-match.1 ~/bin/
