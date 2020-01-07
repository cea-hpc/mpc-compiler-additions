#ifndef HAVE_EXTLS_LOCK_H
#define HAVE_EXTLS_LOCK_H

#include <pthread.h>

typedef pthread_mutex_t extls_lock_t;
#define EXTLS_LOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define extls_lock_init        pthread_mutex_init
#define extls_lock_destroy     pthread_mutex_destroy
#define extls_lock             pthread_mutex_lock
#define extls_unlock           pthread_mutex_unlock
#define extls_trylock          pthread_mutex_trylock


typedef pthread_rwlock_t extls_rwlock_t;
#define EXTLS_RWLOCK_INITIALIZER PTHREAD_RWLOCK_INITIALIZER
#define extls_rwlock_init        pthread_rwlock_init
#define extls_rwlock_destroy     pthread_rwlock_destroy
#define extls_rlock              pthread_rwlock_rdlock
#define extls_runlock            pthread_rwlock_unlock
#define extls_tryrlock           pthread_rwlock_tryrdlock
#define extls_wlock              pthread_rwlock_wrlock
#define extls_wunlock            pthread_rwlock_unlock
#define extls_trywlock           pthread_rwlock_trywrlock

#endif
