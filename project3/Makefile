CC = g++
CC_FLAGS = -Wall -Wextra -Werror -pedantic -g -c
CC_EXTRA = -Wno-unused-parameter

ifdef DEBUG
	CC_EXTRA += -DDEBUG
endif

.PHONY = debug run clean

virtmem: main.o page_table.o disk.o program.o
	$(CC) main.o page_table.o disk.o program.o -o virtmem

main.o: main.cpp
	$(CC) $(CC_FLAGS) $(CC_EXTRA) main.cpp -o main.o

page_table.o: page_table.cpp
	$(CC) $(CC_FLAGS) $(CC_EXTRA) page_table.cpp -o page_table.o

disk.o: disk.cpp
	$(CC) $(CC_FLAGS) disk.cpp -o disk.o

program.o: program.cpp
	$(CC) $(CC_FLAGS) program.cpp -o program.o

debug:
	rm -f *.o virtmem myvirtualdisk
	$(MAKE) DEBUG=1

run: virtmem
	clear
	./virtmem 7 5 rand scan

clean:
	rm -f *.o virtmem myvirtualdisk
