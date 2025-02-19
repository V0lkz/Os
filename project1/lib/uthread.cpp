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

// Signal interupts
static struct itimerval itimer;
static sigset_t block_set;

// TCBs
static TCB *main_thread;
static int tid_num = 1;
static int quantum = -1;

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
    volatile int flag = 0;
    ucontext_t context;
    current_thread->saveContext();
    addToReadyQueue(current_thread);
    current_thread = popFromReadyQueue();
    current_thread->loadContext();
}

// Library functions -----------------------------------------------------------

// The function comments provide an (incomplete) summary of what each library
// function must do

// Starting point for thread. Calls top-level thread function
void stub(void *(*start_routine)(void *), void *arg) {
    start_routine(arg);    // Call start routine
    uthread_exit(0);       // Call exit if start_routine did not
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

    // Initialize itimer data sturcture
    itimer.it_interval.tv_usec = quantum_usecs;    // ??

    // Set up signal handler for SIGVTALRM
    struct sigaction sac;
    if (sigfillset(&sac.sa_mask) != 0) {
        perror("sigfillset");
        return -1;
    }
    sac.sa_flags = 0;
    sac.sa_handler = handle_vtalrm;
    // Install signal handler
    if (sigaction(SIGVTALRM, &sac, NULL) != 0) {
        perror("sigaction");
        return -1;
    }

    return 0;
}

int uthread_create(void *(*start_routine)(void *), void *arg) {
    // Create a new thread and add it to the ready queue
    disableInterrupts();

    // Create new TCB and add to ready queue
    int tid = tid_num++;
    TCB *tcb = new TCB(tid, GREEN, start_routine, arg, READY);
    addToReadyQueue(tcb);

    enableInterrupts();

    return tid;
}

int uthread_join(int tid, void **retval) {
    // If the thread specified by tid is already terminated, just return
    // If the thread specified by tid is still running, block until it terminates
    // Set *retval to be the result of thread if retval != nullptr
}

int uthread_join(int tid, void **retval) {
    // If the thread specified by tid is already terminated, just return
    // If the thread specified by tid is still running, block until it terminates
    // Set *retval to be the result of thread if retval != nullptr
}

int uthread_yield(void) {
    TCB *chosenTCB, *finishTCB;

    disableInterrupts();

    // Choose a TCB from ready queue
    chosenTCB = popFromReadyQueue();
    // Throws exception if empty so need to change later
    if (chosenTCB == NULL) {
        // No threads in queue
        enableInterrupts();
        return -1;
    }

    current_thread->setState(READY);
    addToReadyQueue(current_thread);

    switchThreads();

    current_thread->setState(RUNNING);
    enableInterrupts();

    return 0;
}

void uthread_exit(void *retval) {
    // If this is the main thread, exit the program
    // Move any threads joined on this thread back to the ready queue
    // Move this thread to the finished queue
}

int uthread_suspend(int tid) {
    // Move the thread specified by tid from whatever state it is
    // in to the block queue
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
    // TODO
}

int uthread_get_quantums(int tid) {
    // TODO
}
