CC=gcc
CFLAGS=-I. -Wall -Werror -pthread

all: main

main: obj/main.o obj/heapAllocator.o obj/freeList.o
	$(CC) -o bin/main obj/main.o obj/heapAllocator.o obj/freeList.o

obj/main.o: src/main.c include/heapAllocator.h include/freeList.h
	$(CC) $(CFLAGS) -c src/main.c -o obj/main.o

obj/heapAllocator.o: src/heapAllocator.c include/heapAllocator.h
	$(CC) $(CFLAGS) -c src/heapAllocator.c -o obj/heapAllocator.o

obj/freeList.o: src/freeList.c include/freeList.h
	$(CC) $(CFLAGS) -c src/freeList.c -o obj/freeList.o

run: main
	./bin/main

clean:
	rm -f obj/*.o bin/*