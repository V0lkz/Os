CC = g++
CC_FLAGS = -Wall -Wextra -pedantic -g -c

.PHONY = run clean

virtmem: main.o page_table.o disk.o program.o
	$(CC) main.o page_table.o disk.o program.o -o virtmem

main.o: main.cpp
	$(CC) $(CC_FLAGS) main.cpp -o main.o

page_table.o: page_table.cpp
	$(CC) $(CC_FLAGS) page_table.cpp -o page_table.o

disk.o: disk.cpp
	$(CC) $(CC_FLAGS) disk.cpp -o disk.o

program.o: program.cpp
	$(CC) $(CC_FLAGS) program.cpp -o program.o

run: virtmem
	clear
	./virtmem 7 5 rand scan

clean:
	rm -f *.o virtmem myvirtualdisk
