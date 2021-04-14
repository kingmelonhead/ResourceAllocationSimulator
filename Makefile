CC=gcc
CFLAGS=-g
all : oss proc

oss: oss.c shared.h
	$(CC) -o $@ $^

proc: proc.c shared.h
	$(CC) -o $@ $^

clean:
	rm proc oss