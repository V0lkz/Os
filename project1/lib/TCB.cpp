#include "TCB.h"

#include <ucontext.h>

TCB::TCB(int tid, Priority pr, State state) : _tid(tid), _pr(pr), _state(state) {}

TCB::TCB(int tid, Priority pr, void *(*start_routine)(void *arg), void *arg, State state)
    : _tid(tid), _pr(pr), _quantum(0), _state(state) {
    // Make new context
    if (getcontext(&_context) != 0) {
        perror("getcontext");
        // what should we do here?
        // throw something ?
    }
    _context.uc_stack.ss_sp = new char[STACK_SIZE];
    _context.uc_stack.ss_size = STACK_SIZE;
    _context.uc_stack.ss_flags = 0;
    // Setup thread's stack
    _stack = _context.uc_stack.ss_sp + STACK_SIZE;
    // Push function and args onto stack
    *(_stack) = start_routine;
    _stack -= sizeof(void *);
    *(_stack) = arg;
    _stack -= sizeof(void *);
    // Call make context
    makecontext(&_context, (void (*)()) stub, 2, start_routine, arg);
}

TCB::~TCB() {
    delete _context.uc_stack.ss_sp;
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
