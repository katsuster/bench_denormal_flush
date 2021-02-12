CFLAGS ?= -Wall -g -O2

SRCS = main.c

all:
	$(CC) $(CFLAGS) $(SRCS) -o a.out

clean:
	rm -f a.out
