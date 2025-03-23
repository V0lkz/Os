CC = g++
CFLAGS = -Wall -Wextra -g -lrt --std=c++14
# Remove lrt for MacOS
DEPS = TCB.h uthread.h uthread_private.h Lock.h CondVar.h SpinLock.h async_io.h
OBJ = ./lib/TCB.o ./lib/uthread.o ./lib/Lock.o ./lib/CondVar.o ./lib/SpinLock.o ./lib/async_io.o
OBJ_SOLN = ./solution/TCB_soln.o ./solution/uthread_soln.o ./lib/Lock.o ./lib/CondVar.o ./lib/SpinLock.o ./lib/async_io.o
MAIN_OBJ_UTHRAD_SYNC = ./tests/uthread_sync_demo.o
LOCKPERF_OBJ = ./tests/LockPerformance.o

SERVER_DIR = ./tests/server
GCC = gcc -Wall -Werror -g

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: uthread-sync-demo lockperformance server

uthread-sync-demo-from-soln: $(OBJ_SOLN) $(MAIN_OBJ_UTHRAD_SYNC)
	$(CC) -o uthread-sync-demo $^ $(CFLAGS)

uthread-sync-demo: $(OBJ) $(MAIN_OBJ_UTHRAD_SYNC)
	$(CC) -o $@ $^ $(CFLAGS)

lockperformance: $(OBJ_SOLN) $(LOCKPERF_OBJ)
	$(CC) -o lockperformance $^ $(CFLAGS)

server: http_server concurrent_open.so

http_server: $(OBJ_SOLN) http.o http_server.o connection_queue.o
	$(CC) -Wall -Wextra -g --std=c++14 -o $@ $^

http_server.o: $(SERVER_DIR)/http_server.cpp
	$(CC) -c $< $(CFLAGS)

http.o: $(SERVER_DIR)/http.c $(SERVER_DIR)/http.h
	$(GCC) -c $<

connection_queue.o: $(SERVER_DIR)/connection_queue.cpp $(SERVER_DIR)/connection_queue.h
	$(CC) -c $< $(CFLAGS)

concurrent_open.so: $(SERVER_DIR)/concurrent_open.c
	$(GCC) -shared -fpic -o $@ $^ -ldl

run: server
	./http_server ./tests/server/server_files 8001

.PHONY: clean

clean:
	rm -f ./lib/*.o
	rm -f ./tests/*.o
	rm -f *.o uthread-sync-demo lockperformance
