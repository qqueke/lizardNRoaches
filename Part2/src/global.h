#ifndef GLOBAL_H
#define GLOBAL_H
#include <pthread.h>

extern pthread_mutex_t alphabet_mtx;
extern pthread_mutex_t roach_mtx;
extern pthread_mutex_t lizard_mtx;
extern pthread_mutex_t board_mtx;

extern pthread_mutex_t num_lizards_mtx;
extern pthread_mutex_t num_roaches_mtx;
extern pthread_mutex_t num_wasps_mtx;

#endif // GLOBAL_H