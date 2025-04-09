# Compiler options
CC = g++
CC_FLAGS = -Wall -Wextra -Werror -pedantic -g -c
CC_EXTRA = -Wno-unused-parameter

# MACROs for run-all
# Policies: rand fifo lifo rd_rand wr_rand mrw lrw clock
POLICIES = rand fifo rd_rand wr_rand mrw lrw clock
PROGRAMS = scan sort focus
PAGES = 3
FRAMES = 2

# Newline for run-all
BLANK =
define NEWLINE

$(BLANK)
endef

# Enable DEBUG
ifdef DEBUG
	CC_EXTRA += -DDEBUG
endif

.PHONY = debug run-all clean

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
	rm -f *.o out.txt virtmem myvirtualdisk
	$(MAKE) DEBUG=1

# Ex. make run-all POLICIES="rand fifo clock" PROGRAMS="sort focus"
run-all: virtmem
	clear
	$(foreach POLICY, $(POLICIES), \
	$(foreach PROGRAM, $(PROGRAMS), \
	./virtmem $(PAGES) $(FRAMES) $(POLICY) $(PROGRAM) $(NEWLINE)))

clean:
	rm -f *.o out.txt virtmem myvirtualdisk
