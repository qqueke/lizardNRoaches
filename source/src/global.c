#include "global.h"

/**
 * mutex for lists
*/
pthread_mutex_t alphabet_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t roach_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lizard_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t board_mtx = PTHREAD_MUTEX_INITIALIZER;

/**
 * mutex for constans
*/
pthread_mutex_t num_lizards_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t num_roaches_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t num_wasps_mtx = PTHREAD_MUTEX_INITIALIZER;