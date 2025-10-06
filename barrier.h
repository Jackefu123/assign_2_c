#ifndef BARRIER_H
#define BARRIER_H

#include <pthread.h>

typedef pthread_barrier_t my_barrier_t;

#define my_barrier_init(barrier, count) pthread_barrier_init(barrier, NULL, count)
#define my_barrier_destroy(barrier) pthread_barrier_destroy(barrier)
#define my_barrier_wait(barrier) pthread_barrier_wait(barrier)

#endif // BARRIER_H 
