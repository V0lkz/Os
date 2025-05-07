GCC = /usr/bin/gcc
CFLAGS = -Wall -Wextra

.PHONY = debug lint clean

ifdef DEBUG
	CFLAGS += -DDEBUG
endif

simplefs: shell.o fs.o disk.o
	$(GCC) $(CFLAGS) shell.o fs.o disk.o -o simplefs

shell.o: shell.c
	$(GCC) $(CFLAGS) shell.c -c -o shell.o -g

fs.o: fs.c fs.h
	$(GCC) $(CFLAGS) fs.c -c -o fs.o -g

disk.o: disk.c disk.h
	$(GCC) $(CFLAGS) disk.c -c -o disk.o -g

debug: clean
	$(MAKE) simplefs DEBUG=1

lint:
	clang-format -i fs.c

clean:
	rm -f simplefs disk.o fs.o shell.o
