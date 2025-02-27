#include "uthread.h"

#include <signal.h>
#include <sys/time.h>

#include <cassert>
#include <cerrno>
#include <cstring>
#include <deque>

#include "TCB.h"

// Global Variables  -----------------------------------------------------------

// Queues
static std::deque<TCB *> ready_queue;
static std::deque<TCB *> block_queue;
static std::deque<TCB *> finish_queue;

// Interrupts
static struct itimerval itimer;
static sigset_t block_set;
static int total_quantums;

// TCBs
static int total_threads;
static int tid_num;

static TCB *current_thread;
static TCB *main_thread;

// Interrupt Management --------------------------------------------------------

// Signal handler for SIGVTALRM
static void handle_vtalrm(int signum) {
    // Yield the current thread
    if (uthread_yield() != 0) {
        throw std::runtime_error("uthread_yield");
    }
}

// Start a countdown timer to fire an interrupt
static void startInterruptTimer() {
    if (setitimer(ITIMER_VIRTUAL, &itimer, NULL) != 0) {
        perror("setitimer");
        // Rip
    }
}

// Block signals from firing timer interrupt
static void disableInterrupts() {
    // Add SIGVTALRM to current signal mask
    if (sigprocmask(SIG_BLOCK, &block_set, NULL) != 0) {
        perror("sigprocmask");
        // Rip
    }
}

// Unblock signals to re-enable timer interrupt
static void enableInterrupts() {
    // Remove SIGVTALRM from current signal mask
    if (sigprocmask(SIG_UNBLOCK, &block_set, NULL) != 0) {
        perror("sigprocmask");
        // Rip
    }
}

// Queue Management ------------------------------------------------------------

// Search given queue for specified tid
TCB *getFromQueue(std::deque<TCB *> &queue, int tid) {
    std::deque<TCB *>::iterator iter;
    for (iter = queue.begin(); iter != queue.end(); iter++) {
        if ((*iter)->getId() == tid) {
            return *iter;
        }
    }
    return nullptr;
}

// Add TCB to the back of the given queue
void addToQueue(std::deque<TCB *> &queue, TCB *tcb) {
    queue.push_back(tcb);
}

// Removes and returns the first TCB on the ready queue
// NOTE: Assumes at least one thread on the ready queue
TCB *popFromReadyQueue() {
    TCB *ready_queue_head = ready_queue.front();
    ready_queue.pop_front();
    return ready_queue_head;
}

// Removes the thread specified by the TID provided from the ready queue
// Returns 0 on success, and -1 on failure (thread not in ready queue)
int removeFromQueue(std::deque<TCB *> &queue, int tid) {
    std::deque<TCB *>::iterator iter;
    for (iter = queue.begin(); iter != queue.end(); iter++) {
        if (tid == (*iter)->getId()) {
            queue.erase(iter);
            return 0;
        }
    }
    // Thread not found
    return -1;
}

// Helper functions ------------------------------------------------------------

// Switch to the next ready thread
static int switchThreads() {
    // Flag to keep track of the resumed thread
    volatile int flag = 0;

    // Save old thread context
    if (getcontext(&current_thread->_context) != 0) {
        perror("getcontext");
        return -1;
    }

    // The resumed thread returns here
    if (flag == 1) {
        return 0;
    }

    // Select new thread to run
    current_thread = popFromReadyQueue();
    flag = 1;

    if (setcontext(&current_thread->_context) != 0) {
        addToQueue(ready_queue, current_thread);
        perror("setcontext");
        return -1;
    }
    return 0;
}

// Library functions -----------------------------------------------------------

// Starting point for thread. Calls top-level thread function
void stub(void *(*start_routine)(void *), void *arg) {
    enableInterrupts();                   // Ensure interrupts are enabled
    void *retval = start_routine(arg);    // Call start routine
    uthread_exit(retval);                 // Call exit if start_routine did not
}

int uthread_init(int quantum_usecs) {
    // Initalize empty signal set for block mask
    if (sigemptyset(&block_set) != 0) {
        perror("sigemptyset");
        return -1;
    }

    // Set signal mask to block itimer interupt signal
    if (sigaddset(&block_set, SIGVTALRM) != 0) {
        perror("sigaddset");
        return -1;
    }

    total_quantums = 0;

    // Create TCB for main thread
    // Main thread will have tid 0
    try {
        main_thread = new TCB(0, GREEN, NULL, NULL, RUNNING);
    } catch (const std::exception &e) {
        std::cerr << "TCB: " << e.what() << std::strerror(errno) << std::endl;
        return -1;
    }

    current_thread = main_thread;
    total_threads = 1;
    tid_num = 1;

    // Initialize itimer data sturcture
    itimer.it_interval.tv_sec = quantum_usecs / 1000000;
    itimer.it_interval.tv_usec = quantum_usecs % 1000000;
    itimer.it_value.tv_sec = itimer.it_interval.tv_sec;
    itimer.it_value.tv_usec = itimer.it_interval.tv_usec;

    // Set up signal handler for SIGVTALRM
    struct sigaction sac;
    sac.sa_mask = block_set;
    sac.sa_flags = 0;
    sac.sa_handler = handle_vtalrm;
    if (sigaction(SIGVTALRM, &sac, NULL) != 0) {
        perror("sigaction");
        delete main_thread;
        return -1;
    }

    // Start the timer
    startInterruptTimer();
    return 0;
}

int uthread_create(void *(*start_routine)(void *), void *arg) {
    disableInterrupts();

    // Check if maximun number of threads reached
    if (total_threads >= MAX_THREAD_NUM) {
        enableInterrupts();
        return -1;
    }

    // Arbitrarily assign thread id
    int tid = tid_num++;
    total_threads++;

    // Create new TCB and add to ready queue
    try {
        TCB *tcb = new TCB(tid, GREEN, start_routine, arg, READY);
        addToQueue(ready_queue, tcb);
    } catch (const std::exception &e) {
        std::cerr << "TCB: " << e.what() << ": " << std::strerror(errno) << std::endl;
        return -1;
    }

    enableInterrupts();
    return tid;
}

int uthread_join(int tid, void **retval) {
    disableInterrupts();

    // Check if thread is trying to join itself
    if (current_thread->getId() == tid) {
        enableInterrupts();
        return -1;
    }

    // Check if thread is in the READY queue
    TCB *tcb = getFromQueue(ready_queue, tid);
    if (tcb != nullptr) {
        // Set thread state to BLOCK and add the queue
        current_thread->setState(BLOCK);
        current_thread->setJoinId(tid);
        addToQueue(block_queue, current_thread);
        // Switch threads
        if (switchThreads() != 0) {
            // Failed to switch threads
            current_thread->setState(RUNNING);
            current_thread->setJoinId(-1);
            removeFromQueue(block_queue, current_thread->getId());
            enableInterrupts();
            return -1;
        }
    }

    // Check if thread is in the FINISH queue
    for (auto iter = finish_queue.begin(); iter != finish_queue.end(); iter++) {
        if ((*iter)->getId() == tid) {
            if (retval != nullptr) {
                *retval = (*iter)->getReturnValue();
            }
            delete (*iter);
            finish_queue.erase(iter);
            total_threads--;
            enableInterrupts();
            return 0;
        }
    }

    // Thread not found
    enableInterrupts();
    return -1;
}

int uthread_yield(void) {
    disableInterrupts();

    // Keep track of calling thread for error handling
    TCB *old_thread = current_thread;
    int ret_val = 0;

    // Add current thread to ready queue
    current_thread->setState(READY);
    addToQueue(ready_queue, current_thread);
    current_thread->increaseQuantum();
    total_quantums++;

    // Switch to new thread
    if (switchThreads() != 0) {
        // On failure, resume calling thread
        removeFromQueue(ready_queue, old_thread->getId());
        current_thread = old_thread;
        ret_val = -1;
    }

    // Set thread to RUNNING and start timer
    current_thread->setState(RUNNING);
    startInterruptTimer();

    enableInterrupts();
    return ret_val;
}

void uthread_exit(void *retval) {
    disableInterrupts();

    // Check if calling thread is main thread
    if (current_thread == main_thread) {
        delete current_thread;
        exit(0);
    }

    // Move thread to finish queue
    current_thread->setState(FINISH);
    current_thread->setReturnValue(retval);
    addToQueue(finish_queue, current_thread);

    // Increase thread quantum and total quantums
    current_thread->increaseQuantum();    // Not nessessary
    total_quantums++;

    // Get current thread id
    int current_tid = current_thread->getId();

    // Iterate block queue to check if thread needs to be joined
    std::deque<TCB *>::iterator iter;
    for (iter = block_queue.begin(); iter != block_queue.end(); iter++) {
        if (current_tid == (*iter)->getJoinId()) {
            // Move waiting thread from BLOCK queue to READY queue
            addToQueue(ready_queue, (*iter));
            block_queue.erase(iter);
            break;
        }
    }

    // Switch to a new thread
    if (switchThreads() != 0) {
        // Cannot recover from a thread failing to switch
        throw std::runtime_error("Failed to exit thread");
    }
    enableInterrupts();
}

int uthread_suspend(int tid) {
    // Moves the thread specified by tid into BLOCK queue
    TCB *tcb;
    disableInterrupts();

    // Check if thread is suspending itself
    if (current_thread->getId() == tid) {
        current_thread->setState(BLOCK);
        addToQueue(block_queue, current_thread);
        // Switch to new thread
        int ret_val = 0;
        if (switchThreads() != 0) {
            current_thread->setState(RUNNING);
            removeFromQueue(block_queue, current_thread->getId());
            ret_val = -1;
        }
        enableInterrupts();
        return ret_val;
    }

    // Check READY queue for thread
    if ((tcb = getFromQueue(ready_queue, tid)) != nullptr) {
        removeFromQueue(ready_queue, tid);
        tcb->setState(BLOCK);
        addToQueue(block_queue, tcb);
        enableInterrupts();
        return 0;
    }

    // Check BLOCK queue for thread
    if ((tcb = getFromQueue(block_queue, tid)) != nullptr) {
        // Do nothing if thread is already in BLOCK queue
        enableInterrupts();
        return 0;
    }

    enableInterrupts();
    return -1;
}

int uthread_resume(int tid) {
    // Moves the thread specified by tid into the READY queue
    disableInterrupts();

    // Iterate through BLOCK queue for tid
    std::deque<TCB *>::iterator iter;
    for (iter = block_queue.begin(); iter != block_queue.end(); iter++) {
        if (tid == (*iter)->getId()) {
            (*iter)->setState(READY);
            addToQueue(ready_queue, (*iter));
            block_queue.erase(iter);
            enableInterrupts();
            return 0;
        }
    }

    enableInterrupts();
    return -1;
}

int uthread_once(uthread_once_t *once_control, void (*init_routine)(void)) {
    // init_routine will be ran in the critical section
    disableInterrupts();

    // Check if init_routine has already been executed
    if (once_control->execution_status == UTHREAD_ONCE_NOT_EXECUTED) {
        // Run init_routine and set as executed
        init_routine();
        once_control->execution_status = UTHREAD_ONCE_EXECUTED;
    }

    enableInterrupts();
    return 0;
}

int uthread_self() {
    return current_thread->getId();
}

int uthread_get_total_quantums() {
    return total_quantums;
}

int uthread_get_quantums(int tid) {
    TCB *tcb;
    disableInterrupts();

    // Check RUNNING thread
    if (current_thread->getId() == tid) {
        enableInterrupts();
        return current_thread->getQuantum();
    }

    // Check READY queue
    if ((tcb = getFromQueue(ready_queue, tid)) != nullptr) {
        enableInterrupts();
        return tcb->getQuantum();
    }

    // Check BLOCK queue
    if ((tcb = getFromQueue(block_queue, tid)) != nullptr) {
        enableInterrupts();
        return tcb->getQuantum();
    }

    // Check FINISH queue
    if ((tcb = getFromQueue(finish_queue, tid)) != nullptr) {
        enableInterrupts();
        return tcb->getQuantum();
    }

    enableInterrupts();
    return -1;
}
