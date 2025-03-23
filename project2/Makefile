CC = g++
CFLAGS = -Wall -Wextra -g -lrt --std=c++14
# Remove lrt for MacOS
DEPS = TCB.h uthread.h uthread_private.h Lock.h CondVar.h SpinLock.h async_io.h
OBJ = ./lib/TCB.o ./lib/uthread.o ./lib/Lock.o ./lib/CondVar.o ./lib/SpinLock.o ./lib/async_io.o
OBJ_SOLN = ./solution/TCB_soln.o ./solution/uthread_soln.o ./lib/Lock.o ./lib/CondVar.o ./lib/SpinLock.o ./lib/async_io.o
MAIN_OBJ_UTHRAD_SYNC = ./tests/uthread_sync_demo.o
LOCKPERF_OBJ = ./tests/LockPerformance.o

.PHONY: all run clean

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

all: uthread-sync-demo lockperformance test

uthread-sync-demo-from-soln: $(OBJ_SOLN) $(MAIN_OBJ_UTHRAD_SYNC)
	$(CC) -o uthread-sync-demo $^ $(CFLAGS)

uthread-sync-demo: $(OBJ) $(MAIN_OBJ_UTHRAD_SYNC)
	$(CC) -o $@ $^ $(CFLAGS)

lockperformance: $(OBJ_SOLN) $(LOCKPERF_OBJ)
	$(CC) -o lockperformance $^ $(CFLAGS)

test: $(OBJ_SOLN) ./tests/tests.o
	$(CC) $(CFLAGS) -o $@ $^

run: server
	./http_server ./tests/server/server_files 8000 10000

server:
	make -C ./tests/server

clean:
	rm -f ./lib/*.o
	rm -f ./tests/*.o
	rm -f ./tests/server/*.so ./tests/server/*.o ./tests/server/http_server
	rm -f *.o uthread-sync-demo lockperformance test http_server
