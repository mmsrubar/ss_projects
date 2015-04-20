
CC=gcc
CFLAGS=-Wall -g -pedantic -pthread

fork: fork.c cirbuf.o cirbuf.h
	$(CC) $(CFLAGS) fork.c cirbuf.o cirbuf.h -o fork

cirbuf.o: cirbuf.c cirbuf.h
	$(CC) $(CFLAGS) -c $?

clean:
	rm -f fork core* cirbuf.o

zip:
	zip -r pos3.zip fork.c cirbuf.{c,h} Makefile
