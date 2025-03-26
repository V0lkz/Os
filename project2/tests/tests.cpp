#include <cstdlib>
#include <cstring>
#include <iostream>

#include "../lib/CondVar.h"
#include "../lib/Lock.h"
#include "../lib/SpinLock.h"
#include "../lib/async_io.h"
#include "../lib/uthread.h"

// Test cases
enum tests {
    MUTEX_LOCK = 1,
    SPIN_LOCK,
    COND_VAR,
    MULTI_COND_VAR,
    ASYNC_IO
};

// Busy waiting counter
static volatile unsigned int wait = 0;

/* Testing Setup Functions */

#define NUM_THREADS 5
#define SEED 5103

static void *t_results[NUM_THREADS];    // Thread return value
static int threads[NUM_THREADS];        // Thread id array

// Create threads
int testing_setup(void *(*thread_func)(void *), void *args) {
    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i] = uthread_create(thread_func, args);
        if (threads[i] == -1) {
            std::cerr << "uthread_create" << std::endl;
            return -1;
        }
    }
    return 0;
}

// Join threads
int testing_cleanup() {
    for (int i = 0; i < NUM_THREADS; i++) {
        if (uthread_join(threads[i], &t_results[i]) != 0) {
            std::cerr << "uthread_join" << std::endl;
            return -1;
        }
    }
    return 0;
}

#define BUFLEN 60

// Display test message
void display_test(const char *message) {
    char buf[BUFLEN + 1];
    memset(buf, '=', BUFLEN);
    buf[BUFLEN] = '\0';
    int length = strlen(message);
    int offset = (BUFLEN / 2) - (length / 2);
    buf[offset - 1] = ' ';
    strcpy(buf + offset, message);
    buf[offset + length] = ' ';
    std::cout << buf << std::endl;
}

// Randomly yield thread
inline void random_yield(const int chance) {
    if ((rand() % 100) < chance) {
        uthread_yield();
    }
}

// Busy waiting loop
inline void busy_wait(const int bitstring) {
    while (++wait &= bitstring);
}

/* ====== Test 1: Mutex Lock ====== */

#define NUM_ITER_T1 5

static Lock lock_t1;

static int values_t1[NUM_ITER_T1 * NUM_THREADS];
static int counter_t1 = 0;

void *thread_mutex_lock(void *args) {
    (void) args;
    int tid = uthread_self();
    for (int i = 0; i < NUM_ITER_T1; i++) {
        // Randomly yield after every line to ensure robustness
        random_yield(25);
        lock_t1.lock();
        random_yield(25);
        values_t1[counter_t1] = tid;
        random_yield(25);
        counter_t1++;
        random_yield(25);
        busy_wait(0x7FFFFFF);
        random_yield(25);
        lock_t1.unlock();
        random_yield(25);
    }
    std::cout << "Thread " << tid << " finished" << std::endl;
    return (void *) (long) tid;
}

// Tests Lock::lock() and Lock::unlock()
int test_mutex_lock() {
    display_test("Starting mutex lock test...");
    // Setup threads
    if (testing_setup(thread_mutex_lock, nullptr) != 0) {
        return -1;
    }
    // Join threads
    if (testing_cleanup() != 0) {
        return -1;
    }
    // Check for correctness
    if (counter_t1 != NUM_ITER_T1 * NUM_THREADS) {
        std::cerr << "Counter is incorrect" << std::endl;
        return -1;
    }
    long total_expected = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        total_expected += (long) t_results[i] * NUM_ITER_T1;
    }
    long total_actual = 0;
    for (int i = 0; i < counter_t1; i++) {
        total_actual += values_t1[i];
    }
    if (total_actual != total_expected) {
        std::cerr << "Total sum is incorrect" << std::endl;
        return -1;
    }
    return 0;
}

/* Test 2: SpinLock */

#define NUM_ITER_T2 5

static SpinLock spinlock_t2;

static int values_t2[NUM_ITER_T2 * NUM_THREADS];
static int counter_t2 = 0;

void *thread_spin_lock(void *args) {
    (void) args;
    int tid = uthread_self();
    for (int i = 0; i < NUM_ITER_T1; i++) {
        // Randomly yield after every line to ensure robustness
        random_yield(25);
        spinlock_t2.lock();
        random_yield(25);
        values_t2[counter_t2] = tid;
        random_yield(25);
        counter_t2++;
        random_yield(25);
        spinlock_t2.unlock();
        random_yield(25);
    }
    std::cout << "Thread " << tid << " finished" << std::endl;
    return (void *) (long) tid;
}

// Tests Spinlock::lock() and Spinlock::unlock()
int test_spin_lock() {
    display_test("Starting spinlock test...");
    // Setup threads
    if (testing_setup(thread_spin_lock, nullptr) != 0) {
        return -1;
    }
    // Join threads
    if (testing_cleanup() != 0) {
        return -1;
    }
    // Check for correct results
    if (counter_t2 != NUM_ITER_T2 * NUM_THREADS) {
        std::cerr << "Counter is incorrect" << std::endl;
        return -1;
    }
    long total_expected = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        total_expected += (long) t_results[i] * NUM_ITER_T2;
    }
    long total_actual = 0;
    for (int i = 0; i < counter_t2; i++) {
        total_actual += values_t2[i];
    }
    if (total_actual != total_expected) {
        std::cerr << "Total sum is incorrect" << std::endl;
        return -1;
    }
    return 0;
}

/* Test 3: Condition Variable */

static Lock lock_t3;
static CondVar barrier_cv;

static int waiting_threads = 0;

void *thread_cond_var(void *args) {
    (void) args;
    // Random yielding
    for (int i = 0; i < NUM_THREADS; i++) {
        random_yield(50);
    }
    lock_t3.lock();
    // Broadcast to all threads if its the final thread
    if (waiting_threads == NUM_THREADS - 1) {
        random_yield(33);
        std::cout << "Thread " << uthread_self() << " arrived at barrier" << std::endl;
        barrier_cv.broadcast();
    }
    // Otherwise, check in at barrier
    else {
        random_yield(33);
        waiting_threads++;
        random_yield(33);
        std::cout << "Thread " << uthread_self() << " waiting at barrier" << std::endl;
        barrier_cv.wait(lock_t3);
    }
    lock_t3.unlock();
    std::cout << "Thread " << uthread_self() << " exited barrier" << std::endl;
    return (void *) (long) waiting_threads;
}

// Tests CondVar::wait() and CondVar::broadcast()
int test_cond_var() {
    display_test("Starting condition variable test...");
    // Setup threads
    if (testing_setup(thread_cond_var, nullptr) != 0) {
        return -1;
    }
    // Join threads
    if (testing_cleanup() != 0) {
        return -1;
    }
    // Check for correct results
    for (int i = 0; i < NUM_THREADS; i++) {
        if ((long) t_results[i] != (NUM_THREADS - 1)) {
            std::cerr << "Thread count is incorrect" << std::endl;
            return -1;
        }
    }
    return 0;
}

/* Test 4: Multiple Condition Variables */

static Lock lock_t4;
static CondVar full_cv;
static CondVar empty_cv;

static int buffer_t4[NUM_THREADS];
static int write_idx = 0;
static int read_idx = 0;
static int length = 0;

static bool loop = true;

void *thread_multi_cond_var(void *args) {
    (void) args;
    long items_consumed = 0;
    while (loop) {
        // Randomly yield after every line to ensure robustness
        random_yield(25);
        lock_t4.lock();
        random_yield(25);
        while (length == 0) {
            random_yield(25);
            empty_cv.wait(lock_t4);
            if (!loop) {
                lock_t4.unlock();
                return (void *) items_consumed;
            }
        }
        random_yield(25);
        std::cout << "Thread " << uthread_self() << " read " << buffer_t4[read_idx] << std::endl;
        read_idx++;
        random_yield(25);
        read_idx = read_idx % NUM_THREADS;
        random_yield(25);
        length--;
        random_yield(25);
        items_consumed++;
        random_yield(25);
        full_cv.signal();
        random_yield(25);
        lock_t4.unlock();
        random_yield(25);
    }
    return (void *) items_consumed;
}

// Tests CondVar more thoroughly
int test_multi_cond_var() {
    display_test("Starting multiple condition variables test...");
    // Setup threads
    if (testing_setup(thread_multi_cond_var, nullptr) != 0) {
        return -1;
    }
    // Produce items to add into buffer
    for (int i = 0; i < NUM_THREADS * 2; i++) {
        // Randomly yield after every line to ensure robustness
        random_yield(25);
        lock_t4.lock();
        random_yield(25);
        while (length == NUM_THREADS) {
            random_yield(25);
            full_cv.wait(lock_t4);
        }
        random_yield(25);
        buffer_t4[write_idx] = i;
        random_yield(25);
        write_idx++;
        random_yield(25);
        write_idx = write_idx % NUM_THREADS;
        random_yield(25);
        length++;
        random_yield(25);
        empty_cv.signal();
        random_yield(25);
        lock_t4.unlock();
        random_yield(25);
    }
    // Broadcast to children
    loop = false;
    random_yield(25);
    lock_t4.lock();
    random_yield(25);
    empty_cv.broadcast();
    random_yield(25);
    lock_t4.unlock();
    // Join threads
    if (testing_cleanup() != 0) {
        return -1;
    }
    // Check for correct results
    long total = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        total += (long) t_results[i];
    }
    if (total != NUM_THREADS * 2 || length != 0) {
        std::cerr << "Total items consumed is incorrect" << std::endl;
        return -1;
    }
    return 0;
}

/* ====== Test 5: Asynchronus I/O ====== */

static Lock lock_t5;
static int offset_c = 0;

void *thread_async_io(void *args) {
    int fd = *((int *) args);
    int tid = uthread_self();
    random_yield(25);
    lock_t5.lock();
    random_yield(25);
    std::cout << "Thread " << tid << " writing to pipe" << std::endl;
    if (async_write(fd, &tid, sizeof(int), offset_c) == -1) {
        perror("write");
        return nullptr;
    }
    random_yield(25);
    offset_c += sizeof(int);    // Make sure not to write to same offset
    random_yield(25);
    lock_t5.unlock();
    std::cout << "Thread " << tid << " completed write" << std::endl;
    return nullptr;
}

int test_async_io() {
    display_test("Starting asynchronus I/O test...");
    // Open pipe
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
        perror("pipe");
        return -1;
    }
    // Setup threads
    if (testing_setup(thread_async_io, &pipe_fds[1]) != 0) {
        return -1;
    }
    int nbytes, offset = 0;
    int tids[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        random_yield(25);
        if ((nbytes = async_read(pipe_fds[0], &tids[i], sizeof(int), offset)) == -1) {
            perror("read");
            return -1;
        }
        random_yield(25);
        offset += nbytes;
        random_yield(25);
    }
    // Join threads
    if (testing_cleanup() != 0) {
        return -1;
    }
    // Close pipe
    if (close(pipe_fds[0]) == -1 || close(pipe_fds[1]) == -1) {
        perror("close");
        return -1;
    }
    // Collect results
    if (offset != offset_c) {
        std::cerr << "Offset is incorrect" << std::endl;
        return -1;
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        if (tids[i] != threads[i]) {
            std::cerr << "Read values are incorrect" << std::endl;
            return -1;
        }
        std::cout << tids[i] << " ";
    }
    std::cout << std::endl;
    return 0;
}

/* ======= Main ====== */

int main(int argc, char *argv[]) {
    if (argc != 3 && argc != 4) {
        std::cerr << "Usage: ./test <testnum> <quantum_usecs>" << std::endl;
        std::cerr << "Usage: ./test <testnum> <quantum_usecs> <seed>" << std::endl;
        exit(1);
    }

    const int testnum = atoi(argv[1]);
    const int quantum_usecs = atoi(argv[2]);
    const int seed = (argc == 4 ? atoi(argv[3]) : SEED);
    const bool test_all = (testnum <= 0 ? true : false);

    // Initialize uthread library
    if (uthread_init(quantum_usecs) != 0) {
        std::cerr << "uthread_init" << std::endl;
        exit(1);
    }

    srand(seed);

    // Run tests
    if (test_all || testnum == MUTEX_LOCK) {
        if (test_mutex_lock() != 0) {
            std::cerr << "Mutex lock test failed!" << std::endl;
            exit(1);
        }
        std::cout << "Mutex lock test passed!" << std::endl;
    }
    if (test_all || testnum == SPIN_LOCK) {
        if (test_spin_lock() != 0) {
            std::cerr << "Spinlock test failed!" << std::endl;
            exit(1);
        }
        std::cout << "Spinlock test passed!" << std::endl;
    }
    if (test_all || testnum == COND_VAR) {
        if (test_cond_var() != 0) {
            std::cerr << "Condition variable test failed!" << std::endl;
            exit(1);
        }
        std::cout << "Condition variable test passed!" << std::endl;
    }
    if (test_all || testnum == MULTI_COND_VAR) {
        if (test_multi_cond_var() != 0) {
            std::cerr << "Multiple condition variables test failed!" << std::endl;
            exit(1);
        }
        std::cout << "Multiple condition variables test passed!" << std::endl;
    }
    if (test_all || testnum == ASYNC_IO) {
        if (test_async_io() != 0) {
            std::cerr << "Asynchronus I/O test failed!" << std::endl;
            exit(1);
        }
        std::cout << "Asynchronus I/O test passed!" << std::endl;
    }
    std::cout << "============================================================" << std::endl;
    std::cout << "All executed tests passed!" << std::endl;

    // Exit uthread library
    uthread_exit(nullptr);
}
