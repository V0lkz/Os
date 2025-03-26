#include <chrono>
#include <iostream>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#include "../lib/uthread.h"
#include "../lib/async_io.h"

enum IOType { SYNC, ASYNC };

int file_fd;

struct ThreadArg{
    IOType io_type;
    int num_ops;
    size_t buffer_size;
};

void add_workload(){
    for (int i = 0; i < 10000; ++i) {
        volatile int x = i * i;
    }
}


void *thread_io(void *args){
    int tid = uthread_self();
    ThreadArg *params = (ThreadArg *)args;
    IOType mode = params->io_type;
    int num_ops = params->num_ops;
    char *buffer = new char[params->buffer_size];
    size_t buf_size = params->buffer_size;
   
    memset(buffer, tid, params->buffer_size);
    add_workload();
   
    for(int i = 0; i < num_ops; ++i){
        off_t offset = (tid * num_ops + i) * buf_size;
        if(mode == SYNC){
            //write to completion
            if (pwrite(file_fd, buffer, buf_size, offset) != (ssize_t)buf_size) {
                perror("write");
            }
        }else{
            //allows other threads to run while in io
            if(async_write(file_fd, buffer, buf_size, offset) == -1) {
                perror("async_write");
            };
        }
    }

    return nullptr;
}


void run_test(int num_threads, int num_ops, IOType mode, size_t buf_size ){
    auto start = std::chrono::high_resolution_clock::now();


    int tids[num_threads];
    ThreadArg args = {mode, num_ops, buf_size };
   
    for (int i = 0; i < num_threads; ++i) {
        tids[i] = uthread_create(thread_io, &args);
        if (tids[i] == -1) std::cerr << "uthread_create\n";
    }


    for (int i = 0; i < num_threads; ++i) {
        if (uthread_join(tids[i], nullptr) != 0) {
            std::cerr << "uthread_join\n";
        }
    }


    auto end = std::chrono::high_resolution_clock::now();
    double dur = std::chrono::duration<double, std::milli>(end - start).count();


    std::cout << " completed in " << dur << " ms" << std::endl;
}


int main(int argc, char *argv[]){
    uthread_init(10000);


    file_fd = open("test_io_output.bin", O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (file_fd == -1) {
        perror("open file");
        return 1;
    }


    IOType async_type = ASYNC;
    IOType sync_type = SYNC;

    std::cout << "Test IO with 10 threads and 2 IO operations per threads:\n";
    std::cout << "Testing async IO performance\n";
    run_test(10, 2, async_type, 8192 );
    ftruncate(file_fd, 0);


    std::cout << "Testing sync Performance...\n";
    run_test(10, 2, sync_type, 8192 );
    ftruncate(file_fd, 0);


    std::cout << "Test IO with 20 threads and 1 IO operations per threads:\n";
    std::cout << "Testing async IO performance\n";
    run_test(20, 1, async_type, 8192 * 100);
    ftruncate(file_fd, 0);


    std::cout << "Testing sync Performance...\n";
    run_test(20, 1, sync_type, 8192 * 100);
    ftruncate(file_fd, 0);


    uthread_exit(nullptr);
    return 1;  
}


