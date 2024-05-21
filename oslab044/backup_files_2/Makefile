.PHONY: all
all: zing zing2

zing: zing.o main.o
	gcc -o zing zing.o main.o

zing2: zing2.o main.o
	gcc -o zing2 zing2.o main2.o

main.o: main.c zing.h
	gcc -c main.c -o main.o

.PHONY: clean
clean:
	rm -f main.o zing zing2


