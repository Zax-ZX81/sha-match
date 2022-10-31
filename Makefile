# Makefile for sha-match

CC= gcc
CFLAGS= -I. -Wunused-variable

all: smatch sfind sconvert scheck sdup supdate

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

supdate: supdate.c SMLib.c
	$(CC) -o bin/supdate supdate.c SMLib.c $(CFLAGS)

# install for Linux only
install:
	cp -fv bin/smatch bin/sfind bin/sconvert bin/scheck bin/sdup bin/supdate sha-match.1 ~/bin/
