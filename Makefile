# Makefile for sha-match

CC= gcc
CFLAGS= -I.

all: smatch sfind sconvert

smatch:	smatch.c SMLib.c
	$(CC) -o smatch smatch.c SMLib.c $(CFLAGS)

sfind:	sfind.c SMLib.c
	$(CC) -o sfind sfind.c SMLib.c $(CFLAGS)

sconvert: sconvert.c SMLib.c
	$(CC) -o sconvert sconvert.c SMLib.c $(CFLAGS)

install:
	cp -fv smatch sfind sconvert sha-match.1 ~/bin/
