CC = g++
CFLAGS = -Wall -Wextra -g --std=c++14

# Directories
LIB_DIR = ./../../lib
SOL_DIR = ./../../solution
OUT_DIR = ./../..

# Object files
OBJ_SOLN = $(SOL_DIR)/TCB_soln.o $(SOL_DIR)/uthread_soln.o
OBJ_SYNC = $(LIB_DIR)/Lock.o $(LIB_DIR)/CondVar.o $(LIB_DIR)/async_io.o
OBJ_HTTP = async_socket.o http.o connection_queue.o http_server.o

# HTTP server args
SERVER_FILES = ./server_files
PORT = 8000
QUANTUM = 10000

ifdef DEBUG
	CFLAGS += -DDEBUG
endif

.PHONY: all here debug run run-co clean

all: http_server concurrent_open.so

here:
	$(MAKE) all OUT_DIR="."

debug:
	$(MAKE) here DEBUG=1

$(LIB_DIR)/async_io.o:
	$(CC) $(CFLAGS) -Wno-missing-field-initializers -c -o $@ $(LIB_DIR)/async_io.cpp

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

http_server: $(OBJ_SOLN) $(OBJ_SYNC) $(OBJ_HTTP)
	$(CC) $(CFLAGS) -o $(OUT_DIR)/$@ $^ -lrt

concurrent_open.so: concurrent_open.cpp # $(OBJ_SYNC)
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $^ -ldl

run: http_server
	./http_server $(SERVER_FILES) $(PORT) $(QUANTUM)

# Not fully implemented
run-co: http_server
	LD_PRELOAD=./concurrent_open.so ./http_server $(SERVER_FILES) $(PORT) $(QUANTUM)

clean:
	rm -rf *.o concurrent_open.so http_server
	rm -rf $(LIB_DIR)/*.o
