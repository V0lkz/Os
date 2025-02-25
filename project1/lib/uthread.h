#ifndef _UTHREADS_H
#define _UTHREADS_H
/* From IBM OS 3.1.0 -z/OS C/C++ Runtime Library Reference */
#ifdef _LP64
#define STACK_SIZE 2097152 + 16384 /* large enough value for AMODE 64 */
#else
#define STACK_SIZE 16384 /* AMODE 31 addressing */
#endif

#define MAX_THREAD_NUM 100 /* maximal number of threads */

/* External interface */
typedef enum Priority {
    RED,
    ORANGE,
    GREEN
} Priority;

typedef enum {
    UTHREAD_ONCE_NOT_EXECUTED = 0,
    UTHREAD_ONCE_EXECUTED = 1
} execution_status_t;

typedef struct {
    execution_status_t execution_status;
} uthread_once_t;

#define UTHREAD_ONCE_INIT ((uthread_once_t) {.execution_status = UTHREAD_ONCE_NOT_EXECUTED})

/**
 * Initalize the thread library
 * @param quantum_usecs thread quantum length in us
 * @return 0 on success, -1 on failure
 */
int uthread_init(int quantum_usecs);

/**
 * Create a new thread whose entry point is start_routine
 * @param start_routine function pointer to thread function
 * @param arg pointer to arguments for thread function
 * @return new thread ID on success, -1 on failure
 */
int uthread_create(void *(*start_routine)(void *), void *arg);

/**
 * Join a thread
 * @param tid thread to join
 * @param retval pointer to location to store thread return value
 * @return 0 on success, -1 on failure
 */
int uthread_join(int tid, void **retval);

/**
 * Yield a thread
 * @return 0 on success, -1 on failure
 */
int uthread_yield(void);

/**
 * Terminates calling thread
 *
 * Does not return to caller. If calling thread is the main thread, program is exited.
 * @param retval thread return value
 */
void uthread_exit(void *retval);

/**
 * Suspend a thread
 * @param tid id of thread to suspend
 * @return 0 on succcess, -1 if thread has finished or does not exist
 */
int uthread_suspend(int tid);

/**
 * Resume a thread
 * @param tid if of thread to resume
 * @return 0 on sucess, -1 on failure
 */
int uthread_resume(int tid);

/**
 * Get id of the calling thread
 * @return thread id
 */
int uthread_self(void);

/**
 * Exceute the init_routine only once of a set of threads
 * @param once_control
 * @param init_routine
 * @return 0 if init_routine is executed, 1 on failure
 */
int uthread_once(uthread_once_t *once_control, void (*init_routine)(void));

/**
 * Get the total number of library quantums (times the quantum has been set)
 * @return the total library quantum set count
 */
int uthread_get_total_quantums(void);

/**
 * Get the number of thread quantums for specified thread
 * @return the thread quantum set count
 */
int uthread_get_quantums(int tid);

#endif    // _UTHREADS_H
