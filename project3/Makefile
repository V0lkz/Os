# Compiler options
CC = g++
CC_FLAGS = -Wall -Wextra -Werror -pedantic -g -c
CC_EXTRA = -Wno-unused-parameter

# MACROs for run-all
# Policies: rand fifo lifo rd_rand wr_rand mrw lrw clock
POLICIES = rand fifo lifo rd_rand wr_rand mrw lrw
PROGRAMS = scan sort focus
PAGES = 10
FRAMES = 5

# Newline for run-all
BLANK =
define NEWLINE

$(BLANK)
endef

# Enable DEBUG
ifdef DEBUG
	CC_EXTRA += -DDEBUG
endif

# Enable stderr data collection
# PF: 1, DR: 2, DW: 3
ifdef DATA
	CC_EXTRA += -DDATA=$(DATA)
endif

.PHONY = debug run-all output output-help clean

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

debug: clean
	$(MAKE) DEBUG=1

# Ex. make run-all POLICIES="rand fifo clock" PROGRAMS="sort focus"
run-all: virtmem
	clear
	$(foreach POLICY, $(POLICIES), \
	$(foreach PROGRAM, $(PROGRAMS), \
	./virtmem $(PAGES) $(FRAMES) $(POLICY) $(PROGRAM) $(NEWLINE)))

# Loops for output
LOOPS = 10 20 30 40 50 60 70 80 90 100
OUTFILE = out.txt

output: clean virtmem
	clear
	echo "make output: \n" > $(OUTFILE)
	$(MAKE) output-help >> $(OUTFILE)

output-help:
	$(foreach FRAME, $(LOOPS), \
	$(foreach POLICY, $(POLICIES), \
	$(foreach PROGRAM, $(PROGRAMS), \
	./virtmem 100 $(FRAME) $(POLICY) $(PROGRAM) $(NEWLINE))))

clean:
	rm -f *.o out.txt virtmem myvirtualdisk
