#ifndef BARRIER_H
#define BARRIER_H

#include <pthread.h>

// Define my_barrier_t as pthread_barrier_t
typedef pthread_barrier_t my_barrier_t;

// Define the barrier functions to use pthread barrier functions
#define my_barrier_init(barrier, count) pthread_barrier_init(barrier, NULL, count)
#define my_barrier_destroy(barrier) pthread_barrier_destroy(barrier)
#define my_barrier_wait(barrier) pthread_barrier_wait(barrier)

#endif // BARRIER_H 