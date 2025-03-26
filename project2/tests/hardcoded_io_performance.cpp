#include <chrono>
#include <iostream>
#include <vector>
#include <fcntl.h>
#include <cstdlib>
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
    int num_iter;
};

void add_workload(int num_iteration){
    for (int i = 0; i < num_iteration; ++i) {
        //no complier optimalization
        volatile int x = i * i;
    }
}

void *thread_io(void *args){
    int tid = uthread_self();
    ThreadArg *params = (ThreadArg *)args;
    IOType mode = params->io_type;
    int num_ops = params->num_ops;
    size_t buf_size = params->buffer_size;
    char *buffer = nullptr;
    if (posix_memalign((void **)&buffer, 4096, buf_size) != 0) {
        perror("posix_memalign");
        return nullptr;
    }
   
    memset(buffer, tid, params->buffer_size);
    add_workload(params->num_iter);
    for(int i = 0; i < num_ops; ++i){
        off_t offset = (tid * num_ops + i) * buf_size;
        if(mode == SYNC){
            //write I/O to completion
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
    
    free(buffer);
    return nullptr;
}

void run_test(int num_threads, int num_ops, IOType mode, size_t buf_size, int num_iter = 10000){
    auto start = std::chrono::high_resolution_clock::now();

    int tids[num_threads];
    ThreadArg args = {mode, num_ops, buf_size, num_iter};
    std::vector<ThreadArg> args_vec(num_threads);
    for (int i = 0; i < num_threads; ++i){
        args_vec[i] = {mode, num_ops, buf_size, num_iter};
        tids[i] = uthread_create(thread_io, &args_vec[i]);
        if (tids[i] == -1) std::cerr << "uthread_create\n";
    }

    for (int i = 0; i < num_threads; ++i) {
        if (uthread_join(tids[i], nullptr) != 0) {
            std::cerr << "uthread_join\n";
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    double dur = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout <<"Test IO with " << num_threads << " threads and "<< num_ops 
              << " IO operations per threads" << " with size" << buf_size << " bytes\n"
              << "completed in: " << dur << " ms" << std::endl;
    ftruncate(file_fd, 0);
}

int main(){
    uthread_init(10000);

    file_fd = open("test_io_output.bin", O_CREAT | O_RDWR | O_TRUNC | O_DIRECT, 0644);
    if (file_fd == -1) {
        perror("open file");
        return 1;
    }

    IOType async_type = ASYNC;
    IOType sync_type = SYNC;

    //run_test(number of thread, number of io  operation/thread, size of io, number of iteration in the workload)
    std::cout << "===============================================\n";
    std::cout << "| Test 1 with 10000 iteration in the workload |\n";
    std::cout << "===============================================\n";

    std::cout << "Testing async IO performance\n";
    run_test(10, 2, async_type, 1024 * 8);

    std::cout << "Testing sync Performance...\n";
    run_test(10, 2, sync_type, 1024 * 8 );

    std::cout << "Testing async IO performance\n";
    run_test(10, 2, async_type, 1024 * 32 );

    std::cout << "Testing sync Performance...\n";
    run_test(10, 2, sync_type, 1024 * 32);

    std::cout << "Testing async IO performance\n";
    run_test(10, 2, async_type, 1024 * 64 );

    std::cout << "Testing sync Performance...\n";
    run_test(10, 2, sync_type, 1024 * 64 );


    std::cout << "=================================================\n";
    std::cout << "| Test 2 with 1000 iteration in the workload |\n";
    std::cout << "=================================================\n";
    run_test(10, 5, async_type, 1024 * 8, 1000  );

    std::cout << "Testing sync Performance...\n";
    run_test(10, 5, sync_type, 1024 * 8, 1000  );

    std::cout << "Testing async IO performance\n";
    run_test(10, 5, async_type, 1024 * 32, 1000  );

    std::cout << "Testing sync Performance...\n";
    run_test(10, 5, sync_type, 1024 * 32, 1000  );

    std::cout << "Testing async IO performance\n";
    run_test(10, 5, async_type, 1024 * 64, 1000  );

    std::cout << "Testing sync Performance...\n";
    run_test(10, 5, sync_type, 1024 * 64, 1000  );

    std::cout << "=================================================\n";
    std::cout << "| Test 3 with 100000 iteration in the workload |\n";
    std::cout << "=================================================\n";
    run_test(10, 5, async_type, 1024 * 8, 100000  );

    std::cout << "Testing sync Performance...\n";
    run_test(10, 5, sync_type, 1024 * 8, 100000  );

    std::cout << "Testing async IO performance\n";
    run_test(10, 5, async_type, 1024 * 32, 100000  );

    std::cout << "Testing sync Performance...\n";
    run_test(10, 5, sync_type, 1024 * 32, 100000 );

    std::cout << "Testing async IO performance\n";
    run_test(10, 5, async_type, 1024 * 64, 100000 );

    std::cout << "Testing sync Performance...\n";
    run_test(10, 5, sync_type, 1024 * 64, 100000 );

    std::cout << "Testing async IO performance\n";
    run_test(10, 5, async_type, 1024 * 128, 100000 );

    std::cout << "Testing sync Performance...\n";
    run_test(10, 5, sync_type, 1024 * 128, 100000 );
    
    uthread_exit(nullptr);
    return 1;  
}