/**
 * Thread Control Block Header
 */
#ifndef TCB_H
#define TCB_H
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <ucontext.h>
#include <unistd.h>

#include <iostream>

#include "uthread.h"

extern void stub(void *(*start_routine)(void *), void *arg);

enum State {
    READY,
    RUNNING,
    BLOCK,
    FINISH
};

/**
 * The thread class
 */
class TCB {
public:
    /**
     * Constructor for TCB. Allocate a thread stack and setup the thread
     * context to call the stub function
     * @param tid id for the new thread
     * @param pr priority for the new thread
     * @param f the thread function that get no args and return nothing
     * @param arg the thread function argument
     * @param state current state for the new thread
     */
    TCB(int tid, Priority pr, void *(*start_routine)(void *arg), void *arg, State state);

    /**
     * thread d-tor
     */
    ~TCB();

    /**
     * function to set the thread state
     * @param state the new state for our thread
     */
    void setState(State state);

    /**
     * function that get the state of the thread
     * @return the current state of the thread
     */
    State getState() const;

    /**
     * function that get the ID of the thread
     * @return the ID of the thread
     */
    int getId() const;

    /**
     * function that get the priority of the thread
     * @return the priority of the thread
     */
    Priority getPriority() const;

    /**
     * function to increase the quantum of the thread
     */
    void increaseQuantum();

    /**
     * function that get the quantum of the thread
     * @return the current quantum of the thread
     */
    int getQuantum() const;

    /**
     * function that set and get the return value of the thread
     * @param retval the return value of the thread
     */
    void setReturnValue(void *retval);

    /**
     * function that get the return value of the thread
     */
    void *getReturnValue() const;

    void setJoinId(int tid);

    int getJoinId() const;

    ucontext_t _context;    // The thread's saved context

private:
    int _tid;                              // The thread id number
    Priority _pr;                          // The priority of the thread
    int _quantum;                          // The time interval
    State _state;                          // The state of the thread
    char *_stack;                          // The thread's stack
    int _join_id;                          // Thread id current thread is joining on
    void *_retval;                         // The thread's return value
    void *(*_start_routine)(void *arg);    // The thread's function
};

#endif    // TCB_H
