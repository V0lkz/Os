#include <chrono>
#include <iostream>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#include "../lib/uthread.h"
#include "../lib/async_io.h"

#define NUM_THREADS 4
#define NUM_IO_OPS 100

enum IOType { SYNC, ASYNC };

int pipe_fds[2];

void add_workload() {
    for (int i = 0; i < 1000000; ++i) {
        volatile int x = i * i;
    }
}

void *thread_io(void *args){
    int tid = uthread_self();
    IOType io_type = *(IOType *) args;
    
    for(int i = 0; i < NUM_IO_OPS; ++i){
        if(io_type == SYNC){
            //blocks entire process
            if (write(pipe_fds[1], &tid, sizeof(tid)) != sizeof(tid)) {
                perror("write");
            }
        }else{
            //allows other threads to run while in io
            if(async_write(pipe_fds[1], &tid, sizeof(tid), 0) == -1){
                perror("async_write");
            };
        }
    }
    add_workload();
    return nullptr;
}



void run_test(IOType mode) {
    auto start = std::chrono::high_resolution_clock::now();

    int tids[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; ++i) {
        tids[i] = uthread_create(thread_io, (void*)&mode);
        if (tids[i] == -1) std::cerr << "uthread_create\n";
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        if (uthread_join(tids[i], nullptr) != 0) {
            std::cerr << "uthread_join\n";
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    double dur = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << " completed in " << dur << " ms\n";
}

int main(int argc, char *argv[]){
    uthread_init(100000);

    if(pipe(pipe_fds) == -1) {
        perror("pipe");
        return -1;
    }

    IOType async_type = ASYNC;
    IOType sync_type = SYNC;
    
    std::cout << "Testing async IO performance\n";
    run_test(ASYNC);

    std::cout << "Testing sync Performance...\n";
    run_test(SYNC);

    uthread_exit(nullptr);
    return 1;  
}