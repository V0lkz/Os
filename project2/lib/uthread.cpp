#include "uthread.h"
#include "uthread_private.h"
#include "TCB.h"
#include <vector>
#include <queue>
#include <stdlib.h>
#include <map>
#include <algorithm>
#include <cassert>

using namespace std;

#define NUM_OF_QUEUE 3
#define RED_Q 0
#define ORANGE_Q 1
#define GREEN_Q 2
#define FAIL -1
#define SUCCESS 0
#define MAIN_THREAD 0
#define MICRO_TO_SECOND 1000000
#define THREAD_ERROR "thread library error: "
#define SYS_ERROR "system error: "
#define NOT_FOUND_ID 0
#define SUSPEND_MAIN 1
#define SET_TIME_ERROR 2
#define WRONG_INPUT 3
#define SIGNAL_ACTION_ERROR 4
#define TOO_MANY_THREADS 5
#define BOOST_PRIORITY_FREQUENCY 10

typedef struct join_queue_entry
{
    TCB *tcb;
    int waiting_for_tid;
} join_queue_entry_t;

typedef struct finished_queue_entry
{
    TCB *tcb;
    void *result;
} finished_queue_entry_t;

static vector<TCB *> ready[NUM_OF_QUEUE]; // An array of size 3 of queues, which each represents a queue of priority.
TCB *running;                             // The "Running" thread.
static vector<TCB *> blocked;             // The "Blocked" vector, which represents a queue of threads.
static vector<join_queue_entry_t> join_queue;
static vector<finished_queue_entry_t> finished_queue;
static map<int, TCB *> _threads; // All threads together
static int _quantum_counter = 0;
struct itimerval _timer;
struct sigaction _sigAction;
int *sig;
static int yield_count = 0;

static int removeFromReady(int tid);
static TCB *popReady();
static void _uthread_increase_priority(TCB *tcb);
static void _uthread_decrease_priority(TCB *tcb);

/**
 * function responsible for printing each kind of error
 */
void printError(int type, string pre)
{
    // TODO
}

/*
 * This function finds & returns the next smallest nonnegative integer not already taken by an existing thread,
 * or -1 if there are no available free ids.
 */
int getNextId()
{
    // TODO
    return 0;  // return statement added only to allow compilation (replace with correct code)
}

/**
 * add thread to the requested ready queue
 */
void addToReady(TCB *th)
{
    // TODO
}

/**
 * set time and check if set is done correctly
 */
static void setTime()
{
    // TODO
}

/*
 * returns and remove from Ready the first thread in the queue of the given priority.
 */
TCB *getReady(int pr)
{
    // TODO
    return nullptr;  // return statement added only to allow compilation (replace with correct code)
}

/*
 * returns the thread with the highest priority from Ready and removes it from there.
 * returns NULL in case there are no threads in Ready.
 */
TCB *popReady()
{
    // TODO

    return NULL;  // return statement added only to allow compilation (replace with correct code)
}

/*
 * Translates the priority of the given tid from Enum to int.
 * Returns the int value of the priority.
 */
int translatePriority(int tid)
{
    // TODO
    return 0;  // return statement added only to allow compilation (replace with correct code)
}

/*
 * removes the thread with the given tid from ready.
 */
int removeFromReady(int tid)
{
    // TODO
    return FAIL;  // return statement added only to allow compilation (replace with correct code)
}

/*
 * removes the thread with the given tid from blocked.
 */
void removeFromBlock(int tid)
{
    // TODO
}

/*
 * Removes the thread with the given tid from the finished queue
 * Returns true if the specified tid is in the finished queue, false otherwise
 * Also returns the thread result to retval
 */
bool removeFromFinished(int tid, void **retval)
{
    // TODO
    return false;  // return statement added only to allow compilation (replace with correct code)
}

/*
 * Moves any threads that have joined on tid to the ready queue
 */
void moveFromJoinToReady(int tid)
{
    // TODO
}

// Switch to the thread provided
void switchToThread(TCB *next)
{
    // TODO
}

// Switch to the next thread on the ready queue
void switchThreads()
{
    // TODO
}

void disableInterrupts()
{
    // TODO
}

void enableInterrupts()
{
    // TODO
}

/**
 * switch between running thread and the this thread
 */
static void timeHandler(int signum)
{
    // TODO
}

/*=================================================================================================
 * ======================================Library Functions=========================================
 * ================================================================================================
 */

/* Stub function */
void stub(void *(*start_routine)(void *), void *arg)
{
    // TODO
}

/* Initialize the thread library */
int uthread_init(int quantum_usecs)
{
    // TODO 
    return SUCCESS;  // return statement added only to allow compilation (replace with correct code)
}

/* Create a new thread whose entry point is f */
// int uthread_create(void *(*start_routine)(void), void *arg)
int uthread_create(void *(*start_routine)(void *), void *arg)
{
    // TODO
    return 0;  // return statement added only to allow compilation (replace with correct code)
}

/* Join a thread */
int uthread_join(int tid, void **retval)
{
    // TODO 
    return 0;  // return statement added only to allow compilation (replace with correct code)
}

static void boost_priorities()
{
    // TODO
}

int uthread_yield(void)
{
   // TODO

    return SUCCESS;  // return statement added only to allow compilation (replace with correct code)
}

/* Terminates this thread */
void uthread_exit(void *retval)
{
    // TODO 
}

/* Suspend a thread */
int uthread_suspend(int tid)
{
    // TODO 
    return SUCCESS;
}

/* Resume a thread */
int uthread_resume(int tid)
{
    // TODO
    return SUCCESS;  // return statement added only to allow compilation (replace with correct code)
}

int uthread_once(uthread_once_t *once_control, void (*init_routine)(void))
{
    return SUCCESS;
}

/* Get the id of the calling thread */
int uthread_self()
{
    // TODO
    return 0;  // return statement added only to allow compilation (replace with correct code)
}

/* Get the total number of library quantums */
int uthread_get_total_quantums()
{
    // TODO
    return 0;  // return statement added only to allow compilation (replace with correct code)
}

/* Get the number of thread quantums */
int uthread_get_quantums(int tid)
{
    // TODO
    return 0;  // return statement added only to allow compilation (replace with correct code)
}

// Internal handler for increasing a thread's priority
// NOTE: Assumes interrupts are already disabled
static void _uthread_increase_priority(TCB *tcb)
{
    // TODO 
}

/* Increase the thread's priority by one level */
int uthread_increase_priority(int tid)
{
    // TODO
    return SUCCESS;  // return statement added only to allow compilation (replace with correct code)
}

// Internal handler for decreasing a thread's priority
// NOTE: Assumes interrupts are already disabled
static void _uthread_decrease_priority(TCB *tcb)
{
    // TODO
}

/* Decrease the thread's priority by one level */
int uthread_decrease_priority(int tid)
{
    // TODO
    return SUCCESS;  // return statement added only to allow compilation (replace with correct code)
}
