CC = g++
CFLAGS = -Wall -Wextra -g --std=c++14
# Remove lrt for MacOS

DEPS = TCB.h uthread.h uthread_private.h Lock.h CondVar.h SpinLock.h async_io.h
OBJ = ./lib/TCB.o ./lib/uthread.o ./lib/Lock.o ./lib/CondVar.o ./lib/SpinLock.o ./lib/async_io.o
OBJ_SOLN = ./solution/TCB_soln.o ./solution/uthread_soln.o ./lib/Lock.o ./lib/CondVar.o ./lib/SpinLock.o ./lib/async_io.o
MAIN_OBJ_UTHRAD_SYNC = ./tests/uthread_sync_demo.o

# Run make DEBUG=1 to enable debug statements
ifdef DEBUG
	CFLAGS += -DDEBUG
endif

# Tests
TESTNUM = -1
QUANTUM = 10000

# HTTP Server
SERVER_DIR = ./tests/server
SERVER_FILES = $(SERVER_DIR)/server_files
PORT = 8000    # Run make PORT=# to change port

.PHONY: all debug run-tests run-lock run-io run-server run-server-co clean

all: uthread-sync-demo-from-soln test lockperformance ioperformance server

debug:
	$(MAKE) DEBUG=1

./lib/async_io.o: ./lib/async_io.cpp
	$(CC) $(CFLAGS) -Wno-missing-field-initializers -c -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $^ -lrt

uthread-sync-demo-from-soln: $(OBJ_SOLN) $(MAIN_OBJ_UTHRAD_SYNC)
	$(CC) $(CFLAGS) -o uthread-sync-demo $^ -lrt

uthread-sync-demo: $(OBJ) $(MAIN_OBJ_UTHRAD_SYNC)
	$(CC) $(CFLAGS) -o $@ $^ -lrt

test: $(OBJ_SOLN) ./tests/tests.o
	$(CC) $(CFLAGS) -o $@ $^ -lrt

lockperformance: $(OBJ_SOLN) ./tests/LockPerformance.o
	$(CC) $(CFLAGS) -o $@ $^ -lrt

ioperformance: $(OBJ_SOLN) ./tests/IOPerformance.o
	$(CC) $(CFLAGS) -o $@ $^ -lrt

server:
	$(MAKE) -C $(SERVER_DIR)

# Run function tests
run-tests: test
	./test $(TESTNUM) $(QUANTUM)

# Run lock performance test
run-lock: lockperformance
	./lockperformance $(QUANTUM)

# Run I/O performance test
run-io: ioperformance
	./ioperformance $(QUANTUM)

# Run HTTP server
# Ex. make run-server PORT=8001
run-server: server
	./http_server $(SERVER_FILES) $(PORT) $(QUANTUM)

# Run server concurrency test
# Not fully implemented yet
run-server-co: server
	LD_PRELOAD=$(SERVER_DIR)/concurrent_open.so ./http_server $(SERVER_FILES) $(PORT) $(QUANTUM)

clean:
	rm -f ./lib/*.o
	rm -f ./tests/*.o
	rm -f $(SERVER_DIR)/*.so $(SERVER_DIR)/*.o $(SERVER_DIR)/http_server
	rm -f *.o uthread-sync-demo lockperformance ioperformance test http_server
