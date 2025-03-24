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

# Run make run-server PORT=# to change port
PORT = 8000

.PHONY: all run clean

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $^ -lrt

all: server uthread-sync-demo-from-soln lockperformance test

uthread-sync-demo-from-soln: $(OBJ_SOLN) $(MAIN_OBJ_UTHRAD_SYNC)
	$(CC) $(CFLAGS) -o uthread-sync-demo $^ -lrt

uthread-sync-demo: $(OBJ) $(MAIN_OBJ_UTHRAD_SYNC)
	$(CC) $(CFLAGS) -o $@ $^ -lrt

lockperformance: $(OBJ_SOLN) ./tests/LockPerformance.o
	$(CC) $(CFLAGS) -o $@ $^ -lrt

test: $(OBJ_SOLN) ./tests/tests.o
	$(CC) $(CFLAGS) -o $@ $^ -lrt

server:
	make -C ./tests/server

run-server: server
	./http_server ./tests/server/server_files $(PORT) 10000

run-tests: test
	./test -1 10000

clean:
	rm -f ./lib/*.o
	rm -f ./tests/*.o
	rm -f ./tests/server/*.so ./tests/server/*.o ./tests/server/http_server
	rm -f *.o uthread-sync-demo lockperformance test http_server
