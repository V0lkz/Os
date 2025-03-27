# Compiler options
CC = g++
CFLAGS = -Wall -Wextra -g --std=c++14
# Remove lrt for MacOS

# Object files
DEPS = TCB.h uthread.h uthread_private.h Lock.h CondVar.h SpinLock.h async_io.h
OBJ = ./lib/TCB.o ./lib/uthread.o ./lib/Lock.o ./lib/CondVar.o ./lib/SpinLock.o ./lib/async_io.o
OBJ_SOLN = ./solution/TCB_soln.o ./solution/uthread_soln.o ./lib/Lock.o ./lib/CondVar.o ./lib/SpinLock.o ./lib/async_io.o
MAIN_OBJ_UTHRAD_SYNC = ./tests/uthread_sync_demo.o

# Make with DEBUG=1 to enable debug statements
ifdef DEBUG
	CFLAGS += -DDEBUG
endif

# Unit Tests
TESTNUM = -1
QUANTUM = 10000

# Performance tests
NTHREADS = 10
NITER = 100
NLOOPS = 1000
WORKLOAD = 0

NOPS = 100
OPSIZE = 512

# HTTP Server
SERVER_DIR = ./tests/server
SERVER_FILES = $(SERVER_DIR)/server_files
PORT = 8000    # Run make PORT=# to change port

.PHONY: all debug run-tests run-lock run-io run-server run-server-co io clean

all: uthread-sync-demo-from-soln test lockperformance ioperformance server

debug:
	$(MAKE) clean
	$(MAKE) all DEBUG=1

# Target for async_io b/c -Wmissing-field-initializers
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

lockperformance: $(OBJ_SOLN) ./tests/lock_performance.o
	$(CC) $(CFLAGS) -o $@ $^ -lrt

ioperformance: $(OBJ_SOLN) ./tests/io_performance.o
	$(CC) $(CFLAGS) -o $@ $^ -lrt

hcioperformance: $(OBJ_SOLN) ./tests/hc_io_performance.o
	$(CC) $(CFLAGS) -o $@ $^ -lrt

server:
	$(MAKE) -C $(SERVER_DIR)

# Run function tests
# Ex. make run-tests TESTNUM=5
run-tests: test
	./test $(TESTNUM) $(QUANTUM)

# Run lock performance test
# Ex. make run-lock NTHREADS=20 NLOOPS=100000
run-lock: lockperformance
	./lockperformance $(NTHREADS) $(NITER) $(NLOOPS) $(WORKLOAD) $(QUANTUM)

# Run I/O performance test
run-io: ioperformance
	./ioperformance $(NTHREADS) $(NOPS) $(OPSIZE) $(NITER) $(QUANTUM)

# Run HTTP server
# Ex. make run-server PORT=8001
run-server: server
	./http_server $(SERVER_FILES) $(PORT) $(QUANTUM)

# Run server concurrency test
# Not fully implemented yet (need to compile all library functions with -fPIC)
run-server-co: server
	LD_PRELOAD=$(SERVER_DIR)/concurrent_open.so ./http_server $(SERVER_FILES) $(PORT) $(QUANTUM)

# Runs hard coded performance tests for write up
io:
	$(MAKE) clean
	$(MAKE) hcioperformance
	./hcioperformance

clean:
	rm -f ./lib/*.o
	rm -f ./tests/*.o
	rm -f $(SERVER_DIR)/*.so $(SERVER_DIR)/*.o $(SERVER_DIR)/http_server
	rm -f *.o uthread-sync-demo lockperformance hcioperformance ioperformance ioperformance.txt test http_server
