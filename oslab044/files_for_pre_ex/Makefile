CC := gcc


.PHONY: all
all: zing zing2

zing: zing.o main.o
	$(CC) -o $@ $^
zing2: zing2.o main.o
	$(CC) -o $@ $^

%.o: %.c main.c zing.h zing2.h
	$(CC) -c $< -o $@

.PHONY: clean
clean:
	rm -f main.o zing zing2


