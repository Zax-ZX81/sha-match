# Makefile for sha-match

CC= gcc
CFLAGS= -I.

all: smatch sfind sconvert

smatch:	smatch.c SMLib.c
	$(CC) -o bin/smatch smatch.c SMLib.c $(CFLAGS)

sfind:	sfind.c SMLib.c
	$(CC) -o bin/sfind sfind.c SMLib.c $(CFLAGS)

sconvert: sconvert.c SMLib.c
	$(CC) -o bin/sconvert sconvert.c SMLib.c $(CFLAGS)

install:
	cp -fv bin/* sha-match.1 ~/bin/
