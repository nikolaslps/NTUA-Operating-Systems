#
# Makefile
#

CC = gcc
CFLAGS = -Wall -O2 -pthread

all: ask3 ask3final

ask3: ask3.o help.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $<
ask3final: ask3final.c
	$(CC) $(CFLAGS) $^ -o $@
clean:
	rm -f *.o ask3
