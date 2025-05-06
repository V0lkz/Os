GCC = /usr/bin/gcc
CFLAGS = -Wall -Wextra -Werror

.PHONY = lint clean

simplefs: shell.o fs.o disk.o
	$(GCC) $(CFLAGS) shell.o fs.o disk.o -o simplefs

shell.o: shell.c
	$(GCC) $(CFLAGS) shell.c -c -o shell.o -g

fs.o: fs.c fs.h
	$(GCC) $(CFLAGS) fs.c -c -o fs.o -g

disk.o: disk.c disk.h
	$(GCC) $(CFLAGS) disk.c -c -o disk.o -g

lint:
	clang-format -i fs.c

clean:
	rm -f simplefs disk.o fs.o shell.o
