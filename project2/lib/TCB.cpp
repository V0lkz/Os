/*
 *
 */

#include "TCB.h"

#include <cassert>

TCB::TCB(int tid, Priority pr, void *(*start_routine)(void *arg), void *arg, State state)
    : _tid(tid), _pr(pr), _quantum(0), _state(state), _lock_count(0) {
    // TODO
}

TCB::~TCB() {
    // TODO
}

void TCB::setState(State state) {
    // TODO
}

State TCB::getState() const {
    // TODO
    return State::RUNNING;    // return statement added only to allow compilation (replace with
                              // correct code)
}

int TCB::getId() const {
    // TODO
    return 0;    // return statement added only to allow compilation (replace with correct code)
}

Priority TCB::getPriority() const {
    // TODO
    return Priority::GREEN;    // return statement added only to allow compilation (replace with
                               // correct code)
}

void TCB::increaseQuantum() {
    // TODO
}

int TCB::getQuantum() const {
    // TODO
    return 0;    // return statement added only to allow compilation (replace with correct code)
}

void TCB::increaseLockCount() {
    // TODO
}

void TCB::decreaseLockCount() {
    // TODO
}

int TCB::getLockCount() {
    // TODO
    return 0;    // return statement added only to allow compilation (replace with correct code)
}

void TCB::increasePriority() {
    // TODO
}

void TCB::decreasePriority() {
    // TODO
}
