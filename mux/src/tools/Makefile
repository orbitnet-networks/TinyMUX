CC = gcc

all: announce

announce: announce.c
	$(CC) -o announce announce.c

realclean:
	-rm -f *.o a.out core gmon.out announce autoconf.h Makefile

clean:
	-rm -f *.o a.out core gmon.out
