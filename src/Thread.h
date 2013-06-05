/*
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.
 */


#ifndef THREAD_INCLUDED
#define THREAD_INCLUDED
#include <pthread.h>

/**
 * This interface contains <b>Thread</b> and <b>Mutex</b> abstractions 
 * via Macros.
 *
 * @file
 */

#define Thread_T pthread_t
#define Sem_T   pthread_cond_t			  
#define Mutex_T pthread_mutex_t
#define ThreadData_T pthread_key_t
#define wrapper(F) do { int status=F; \
        if (status!=0 && status!=ETIMEDOUT) \
                ABORT("Thread: %s\n", System_getError(status)); \
        } while (0)
#define Thread_create(thread, threadFunc, threadArgs) \
        wrapper(pthread_create(&thread, NULL, threadFunc, (void*)threadArgs))
#define Thread_self() pthread_self()
#define Thread_detach(thread) wrapper(pthread_detach(thread))
#define Thread_cancel(thread) wrapper(pthread_cancel(thread))
#define Thread_join(thread) wrapper(pthread_join(thread, NULL))
#define Sem_init(sem) wrapper(pthread_cond_init(&sem, NULL))
#define Sem_wait(sem, mutex) wrapper(pthread_cond_wait(&sem, &mutex))
#define Sem_signal(sem) wrapper(pthread_cond_signal(&sem))
#define Sem_broadcast(sem) wrapper(pthread_cond_broadcast(&sem))
#define Sem_destroy(sem) wrapper(pthread_cond_destroy(&sem))
#define Sem_timeWait(sem, mutex, time) \
        wrapper(pthread_cond_timedwait(&sem, &mutex, &time))
#define Mutex_init(mutex) wrapper(pthread_mutex_init(&mutex, NULL))
#define Mutex_destroy(mutex) wrapper(pthread_mutex_destroy(&mutex))
#define Mutex_lock(mutex) wrapper(pthread_mutex_lock(&mutex))
#define Mutex_unlock(mutex) wrapper(pthread_mutex_unlock(&mutex))
#define LOCK(mutex) do { Mutex_T *_yymutex=&(mutex); \
        wrapper(pthread_mutex_lock(_yymutex));
#define END_LOCK wrapper(pthread_mutex_unlock(_yymutex)); } while (0)
#define ThreadData_create(key) wrapper(pthread_key_create(&(key), NULL))
#define ThreadData_set(key, value) pthread_setspecific((key), (value))
#define ThreadData_get(key) pthread_getspecific((key))

#endif
