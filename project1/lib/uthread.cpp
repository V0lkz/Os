#include "uthread.h"

#include <signal.h>
#include <sys/time.h>

#include <cassert>
#include <deque>

#include "TCB.h"

using namespace std;

// Finished queue entry type
typedef struct finished_queue_entry {
    TCB *tcb;        // Pointer to TCB
    void *result;    // Pointer to thread result (output)
} finished_queue_entry_t;

// Join queue entry type
typedef struct join_queue_entry {
    TCB *tcb;               // Pointer to TCB
    int waiting_for_tid;    // TID this thread is waiting on
} join_queue_entry_t;

// You will need to maintain structures to track the state of threads
// - uthread library functions refer to threads by their TID so you will want
//   to be able to access a TCB given a thread ID
// - Threads move between different states in their lifetime (READY, BLOCK,
//   FINISH). You will want to maintain separate "queues" (doesn't have to
//   be that data structure) to move TCBs between different thread queues.
//   Starter code for a ready queue is provided to you
// - Separate join and finished "queues" can also help when supporting joining.
//   Example join and finished queue entry types are provided above

// Global Variables  -----------------------------------------------------------

// Queues
static std::deque<TCB *> ready_queue;
static std::deque<TCB *> finish_queue;
static std::deque<TCB *> block_queue;

// Interrupts
static struct itimerval itimer;
static sigset_t block_set;
static int quantum;

// TCBs
static int total_threads = 0;
static int tid_num = 1;

static TCB *main_thread;
static TCB *current_thread;

// Interrupt Management --------------------------------------------------------

// Signal handler for SIGVTALRM
static void handle_vtalrm(int signum) {
    uthread_yield();
}

// Start a countdown timer to fire an interrupt
static void startInterruptTimer() {
    setitimer(ITIMER_VIRTUAL, &itimer, NULL);
}

// Block signals from firing timer interrupt
static void disableInterrupts() {
    // Add SIGVTALRM to current signal mask
    if (sigprocmask(SIG_BLOCK, &block_set, NULL) != 0) {
        perror("sigprocmask");
        // rip
    }
}

// Unblock signals to re-enable timer interrupt
static void enableInterrupts() {
    // Remove SIGVTALRM from current signal mask
    if (sigprocmask(SIG_UNBLOCK, &block_set, NULL) != 0) {
        perror("sigprocmask");
        // rip
    }
}

// Queue Management ------------------------------------------------------------

// Add TCB to the back of the ready queue
void addToReadyQueue(TCB *tcb) {
    ready_queue.push_back(tcb);
}

// Removes and returns the first TCB on the ready queue
// NOTE: Assumes at least one thread on the ready queue
TCB *popFromReadyQueue() {
    assert(!ready_queue.empty());

    TCB *ready_queue_head = ready_queue.front();
    ready_queue.pop_front();
    return ready_queue_head;
}

// Removes the thread specified by the TID provided from the ready queue
// Returns 0 on success, and -1 on failure (thread not in ready queue)
int removeFromReadyQueue(int tid) {
    for (deque<TCB *>::iterator iter = ready_queue.begin(); iter != ready_queue.end(); ++iter) {
        if (tid == (*iter)->getId()) {
            ready_queue.erase(iter);
            return 0;
        }
    }
    // Thread not found
    return -1;
}

// Helper functions ------------------------------------------------------------

// Switch to the next ready thread
static void switchThreads() {
    // flag, keep track of the resumed thread
    volatile int flag = 0;

    // Save old thread context
    if (current_thread->saveContext() != 0) {
        perror("getcontext");
        return;
    }

    // The resumed thread return here
    if (flag == 1) {
        return;
    }

    // Select new thread to run
    current_thread = popFromReadyQueue();
    current_thread->loadContext();
    flag = 1;
}

// Library functions -----------------------------------------------------------

// The function comments provide an (incomplete) summary of what each library
// function must do

// Starting point for thread. Calls top-level thread function
void stub(void *(*start_routine)(void *), void *arg) {
    void *retval = start_routine(arg);    // Call start routine
    uthread_exit(retval);                 // Call exit if start_routine did not
}

int uthread_init(int quantum_usecs) {
    // Initialize any data structures
    // Setup timer interrupt and handler
    // Create a thread for the caller (main) thread

    // Set signal mask to block itimer interupt signal
    if (sigaddset(&block_set, SIGVTALRM) != 0) {
        perror("sigaddset");
        return -1;
    }

    quantum = quantum_usecs;

    // Create TCB for main thread
    // Main thread will have tid 0
    main_thread = new TCB(0, GREEN, NULL, NULL, READY);
    current_thread = main_thread;
    total_threads = 1;

    // Initialize itimer data sturcture
    itimer.it_value.tv_sec = quantum_usecs / 1000000;
    itimer.it_value.tv_usec = quantum_usecs % 1000000;
    itimer.it_interval.tv_sec = quantum_usecs / 1000000;
    itimer.it_interval.tv_usec = quantum_usecs % 1000000;

    // Set up signal handler for SIGVTALRM
    struct sigaction sac;
    if (sigfillset(&sac.sa_mask) != 0) {
        perror("sigfillset");
        return -1;
    }
    sac.sa_flags = 0;
    sac.sa_handler = handle_vtalrm;
    if (sigaction(SIGVTALRM, &sac, NULL) != 0) {
        perror("sigaction");
        return -1;
    }

    // Start the timer
    startInterruptTimer();
    return 0;
}

int uthread_create(void *(*start_routine)(void *), void *arg) {
    // Create a new thread and add it to the ready queue
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
    TCB *tcb = new TCB(tid, GREEN, start_routine, arg, READY);
    addToReadyQueue(tcb);

    enableInterrupts();

    return tid;
}

int uthread_join(int tid, void **retval) {
    // If the thread specified by tid is already terminated, just return
    // If the thread specified by tid is still running, block until it terminates
    // Set *retval to be the result of thread if retval != nullptr

    disableInterrupts();

    std::deque<TCB *>::iterator iter;
    for (iter = finish_queue.begin(); iter != finish_queue.end(); iter++) {
        if ((*iter)->getId() == tid) {
            *retval = (*iter)->getReturnValue();
            return 0;
        }
    }

    int found = 0;
    for (iter = ready_queue.begin(); iter != ready_queue.end(); iter++) {
        if ((*iter)->getId() == tid) {
            found = 1;
            uthread_yield();
            break;
        }
    }

    enableInterrupts();

    return 0;
}

int uthread_yield(void) {
    // TCB *chosenTCB;

    disableInterrupts();

    // // Choose a TCB from ready queue
    // chosenTCB = popFromReadyQueue();
    // // Throws exception if empty so need to change later
    // // Shouldn't be empty because of main thread
    // if (chosenTCB == NULL) {
    //     // No threads in queue
    //     enableInterrupts();
    //     return -1;
    // }

    // Add current thread to ready queue
    current_thread->setState(READY);
    addToReadyQueue(current_thread);

    // Switch to new thread
    switchThreads();

    startInterruptTimer();
    current_thread->setState(RUNNING);

    enableInterrupts();

    return current_thread->getId();
}

void uthread_exit(void *retval) {
    // If this is the main thread, exit the program
    // Move any threads joined on this thread back to the ready queue
    // Move this thread to the finished queue

    // current_thread->setState(FINISH);
    current_thread->setReturnValue(retval);
}

int uthread_suspend(int tid) {
    // Move the thread specified by tid from whatever state it is
    // in to the block queue

    disableInterrupts();

    // get from ready queue
    // remove from ready queue
    // add to block queue

    enableInterrupts();
    return 0;
}

int uthread_resume(int tid) {
    // Move the thread specified by tid back to the ready queue
}

int uthread_once(uthread_once_t *once_control, void (*init_routine)(void)) {
    // Use the once_control structure to determine whether or not to execute
    // the init_routine
    // Pay attention to what needs to be accessed and modified in a critical
    // region (critical meaning interrupts disabled)
}

int uthread_self() {
    return current_thread->getId();
}

int uthread_get_total_quantums() {
    int total = 0;

    disableInterrupts();

    // Iterate through READY queue and total up quantums
    std::deque<TCB *>::iterator iter;
    for (iter = finish_queue.begin(); iter != finish_queue.end(); iter++) {
        total += (*iter)->getQuantum();
    }

    // Add quantom for current RUNNING thread
    total += current_thread->getQuantum();

    enableInterrupts();

    return total;
}

int uthread_gt_quantums(int tid) {
    disableInterrupts();
    // Iterate through READY Queue for given tid
    std::deque<TCB *>::iterator iter;
    for (iter = finish_queue.begin(); iter != finish_queue.end(); iter++) {
        if ((*iter)->getId() == tid) {
            enableInterrupts();
            return (*iter)->getQuantum();
        }
    }
    enableInterrupts();
    return -1;
}
