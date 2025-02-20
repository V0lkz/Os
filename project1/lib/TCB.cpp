#include "TCB.h"

#include <ucontext.h>

extern int quantum;

TCB::TCB(int tid, Priority pr, void *(*start_routine)(void *arg), void *arg, State state)
    : _tid(tid), _pr(pr), _quantum(0), _state(state), _retval(NULL) {
    // Make new context
    if (getcontext(&_context) != 0) {
        perror("getcontext");
        // rip
    }
    _context.uc_stack.ss_sp = new char[STACK_SIZE];
    _context.uc_stack.ss_size = STACK_SIZE;
    _context.uc_stack.ss_flags = 0;
    // Setup thread's stack
    _stack = (char *) (_context.uc_stack.ss_sp) + STACK_SIZE;
    // Continue constructing if it is not the main thread
    if (tid != 0) {
    
        //push start_routine and arg onto the stack   
        //store the routine and arg in memeory 
        _stack -= sizeof(void *);
        *(void **)_stack = (void *)start_routine;
    
        _stack -= sizeof(void *);
        *(void **)_stack = (void *)arg;
        // Call make context
        makecontext(&_context, (void (*)())(stub), 2, start_routine, arg);
    }
}

TCB::~TCB() {
    delete (char *) _context.uc_stack.ss_sp;
}

void TCB::setState(State state) {
    _state = state;
}

State TCB::getState() const {
    return _state;
}

int TCB::getId() const {
    return _tid;
}

void TCB::increaseQuantum() {
    _quantum++;
}

int TCB::getQuantum() const {
    return _quantum;
}

int TCB::saveContext() {
    return getcontext(&_context);
}

void TCB::loadContext() {
    setcontext(&_context);
}

void TCB::setReturnValue(void *retval) {
    _retval = retval;
}

void *TCB::getReturnValue() const {
    return _retval;
}
