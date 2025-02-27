#include "TCB.h"

#include <ucontext.h>

#include <exception>

TCB::TCB(int tid, Priority pr, void *(*start_routine)(void *arg), void *arg, State state)
    : _tid(tid), _pr(pr), _quantum(0), _state(state), _join_id(-1), _retval(NULL) {
    // Save current context
    if (getcontext(&_context) != 0) {
        throw std::runtime_error("getcontext");
    }
    if (tid != 0) {
        // Create stack for thread
        void *stack_ptr;
        if (posix_memalign(&stack_ptr, 16, STACK_SIZE) != 0) {
            throw std::runtime_error("posix_memalign");
        }
        _context.uc_stack.ss_sp = stack_ptr;
        _context.uc_stack.ss_size = STACK_SIZE;
        _context.uc_stack.ss_flags = 0;
        _context.uc_link = NULL;
        // Setup TCB stack pointer
        _stack = static_cast<char *>(_context.uc_stack.ss_sp);
        // Continue constructing if it is not the main thread
        makecontext(&_context, (void (*)())(stub), 2, start_routine, arg);
    }
}

TCB::~TCB() {
    // Free stack if not the main thread
    if (_tid != 0) {
        free(_stack);
    }
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

void TCB::setReturnValue(void *retval) {
    _retval = retval;
}

void *TCB::getReturnValue() const {
    return _retval;
}

void TCB::setJoinId(int tid) {
    _join_id = tid;
}

int TCB::getJoinId() const {
    return _join_id;
}
