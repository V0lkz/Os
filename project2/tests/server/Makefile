CFLAGS = -Wall -Wextra -g --std=c++14
CC = g++

HOME_DIR = ./../../
OBJ_SOLN = $(HOME_DIR)solution/TCB_soln.o $(HOME_DIR)solution/uthread_soln.o $(HOME_DIR)lib/Lock.o $(HOME_DIR)lib/CondVar.o $(HOME_DIR)/lib/async_io.o

ifdef DEBUG
	CFLAGS += -DDEBUG
endif

.PHONY: all clean

all: http_server concurrent_open.so

$(HOME_DIR)/lib/async_io.o:
	$(CC) $(CFLAGS) -Wno-missing-field-initializers -c -o $@ $(HOME_DIR)/lib/async_io.cpp

barrier.o: barrier.cpp
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

http_server: $(OBJ_SOLN) async_socket.o http.o connection_queue.o http_server.o
	$(CC) $(CFLAGS) -o $(HOME_DIR)$@ $^ -lrt

concurrent_open.so: concurrent_open.c barrier.o
	gcc -Wall -Wextra -g -shared -fPIC -o $@ $^ -ldl

clean:
	rm -rf *.o concurrent_open.so http_server
	rm -rf ./../../lib/*.o
