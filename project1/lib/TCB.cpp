#include "TCB.h"
#include <ucontext.h>

TCB::TCB(int tid, Priority pr, void *(*start_routine)(void *arg), void *arg,
         State state) {
  getcontext(&_context);
  _context.uc_stack.ss_sp = new char[STACK_SIZE];
  _context.uc_stack.ss_size = STACK_SIZE;
  _context.uc_stack.ss_flags = 0;
  makecontext(&_context, (void (*)())stub, 2, start_routine, arg);
}

TCB::~TCB() {}

void TCB::setState(State state) { _state = state; }

State TCB::getState() const { return _state; }

int TCB::getId() const { return _tid; }

void TCB::increaseQuantum() {}

int TCB::getQuantum() const {}

int TCB::saveContext() { return getcontext(&_context); }

void TCB::loadContext() { setcontext(&_context); }
