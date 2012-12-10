
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
 */


#include "Config.h"

#include <stdio.h>
#include <string.h>

#include "URL.h"
#include "Thread.h"
#include "system/Time.h"
#include "Vector.h"
#include "ResultSet.h"
#include "PreparedStatement.h"
#include "Connection.h"
#include "ConnectionPool.h"


/**
 * Implementation of the ConnectionPool interface
 *
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#define T ConnectionPool_T
struct ConnectionPool_S {
        URL_T url;
        int filled;
        int doSweep;
        char *error;
        Sem_T alarm;
	Mutex_T mutex;
	Vector_T pool;
        Thread_T reaper;
        int sweepInterval;
	int maxConnections;
        volatile int stopped;
        int connectionTimeout;
	int initialConnections;
};

int ZBDEBUG = false;
#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif
void(*AbortHandler)(const char *error) = NULL;
#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif


/* ------------------------------------------------------- Private methods */


static void drainPool(T P) {
        while (! Vector_isEmpty(P->pool)) {
		Connection_T con = Vector_pop(P->pool);
		Connection_free(&con);
	}
}


static int fillPool(T P) {
	for (int i = 0; i < P->initialConnections; i++) {
                Connection_T con = Connection_new(P, &P->error);
		if (! con) {
                        if (i > 0) {
                                DEBUG("Failed to fill the pool with initial connections -- %s\n", P->error);
                                FREE(P->error);
                                return true;
                        }
                        return false;
                }
		Vector_push(P->pool, con);
	}
	return true;
}


static int getActive(T P){
        int i, n = 0, size = Vector_size(P->pool);
        for (i = 0; i < size; i++)
                if (! Connection_isAvailable(Vector_get(P->pool, i))) 
                        n++; 
        return n; 
}


static int reapConnections(T P) {
        int n = 0;
        int x = Vector_size(P->pool) - getActive(P) - P->initialConnections;
        time_t timedout = Time_now() - P->connectionTimeout;
        for (int i = 0; ((n < x) && (i < Vector_size(P->pool))); i++) {
                Connection_T con = Vector_get(P->pool, i);
                if (Connection_isAvailable(con)) {
                        if ((! Connection_ping(con)) || (Connection_getLastAccessedTime(con) < timedout)) {
                                Vector_remove(P->pool, i);
                                Connection_free(&con);
                                n++;
                                i--;
                        }
                } 
        }
        return n;
}


static void *doSweep(void *args) {
        T P = args;
        struct timespec wait = {0, 0};
        Mutex_lock(P->mutex);
        while (! P->stopped) {
                wait.tv_sec = Time_now() + P->sweepInterval;
                Sem_timeWait(P->alarm,  P->mutex, wait);
                if (P->stopped) break;
                reapConnections(P);
        }
        Mutex_unlock(P->mutex);
        DEBUG("Reaper thread stopped\n");
        return NULL;
}


/* ---------------------------------------------------------------- Public */


T ConnectionPool_new(URL_T url) {
        T P;
	if (! url)
                return NULL;
#ifdef ZILD_PACKAGE_PROTECTED
        Exception_init();
#endif
	NEW(P);
        P->url = url;
	Mutex_init(P->mutex);
	P->maxConnections = SQL_DEFAULT_MAX_CONNECTIONS;
        P->pool = Vector_new(SQL_DEFAULT_MAX_CONNECTIONS);
	P->initialConnections = SQL_DEFAULT_INIT_CONNECTIONS;
        P->connectionTimeout = SQL_DEFAULT_CONNECTION_TIMEOUT;
	return P;
}


void ConnectionPool_free(T *P) {
        Vector_T pool;
	assert(P && *P);
        pool = (*P)->pool;
        if (! (*P)->stopped)
                ConnectionPool_stop((*P));
        Vector_free(&pool);
	Mutex_destroy((*P)->mutex);
        FREE((*P)->error);
	FREE(*P);
}


/* ------------------------------------------------------------ Properties */


URL_T ConnectionPool_getURL(T P) {
        assert(P);
        return P->url;
}


void ConnectionPool_setInitialConnections(T P, int connections) {
        assert(P);
        assert(connections >= 0);
        LOCK(P->mutex)
        {
                P->initialConnections = connections;
        }
        END_LOCK;
}


int ConnectionPool_getInitialConnections(T P) {
        assert(P);
        return P->initialConnections;
}


void ConnectionPool_setMaxConnections(T P, int maxConnections) {
        assert(P);
        assert(P->initialConnections <= maxConnections);
        LOCK(P->mutex)
        {
                P->maxConnections = maxConnections;
        }
        END_LOCK;
}


int ConnectionPool_getMaxConnections(T P) {
        assert(P);
        return P->maxConnections;
}


void ConnectionPool_setConnectionTimeout(T P, int connectionTimeout) {
        assert(P);
        assert(connectionTimeout > 0);
        P->connectionTimeout = connectionTimeout;
}


int ConnectionPool_getConnectionTimeout(T P) {
        assert(P);
        return P->connectionTimeout;
}


void ConnectionPool_setAbortHandler(T P, void(*abortHandler)(const char *error)) {
        assert(P); 
        AbortHandler = abortHandler;
}


void ConnectionPool_setReaper(T P, int sweepInterval) {
        assert(P);
        assert(sweepInterval>0);
        LOCK(P->mutex)
        {
                P->doSweep = true;
                P->sweepInterval = sweepInterval;
        }
        END_LOCK;
}


int ConnectionPool_size(T P) {
        assert(P);
        return Vector_size(P->pool);
}


int ConnectionPool_active(T P) {
        int n = 0;
        assert(P);
        LOCK(P->mutex)
        {
                n = getActive(P);
        }
        END_LOCK;
        return n;
}


/* -------------------------------------------------------- Public methods */


void ConnectionPool_start(T P) {
        assert(P);
        LOCK(P->mutex)
        {
                P->stopped = false;
                if (! P->filled) {
                        P->filled = fillPool(P);
                        if (P->filled && P->doSweep) {
                                DEBUG("Starting Database reaper thread\n");
                                Sem_init(P->alarm);
                                Thread_create(P->reaper, doSweep, P);
                        }
                }
        }
        END_LOCK;
        if (! P->filled)
                THROW(SQLException, "Failed to start connection pool -- %s", P->error);
}


void ConnectionPool_stop(T P) {
        int stopSweep = false;
        assert(P);
        LOCK(P->mutex)
        {
                P->stopped = true;
                if (P->filled) {
                        drainPool(P);
                        P->filled = false;
                        stopSweep = (P->doSweep && P->reaper);
                        Connection_onstop(P);
                }
        }
        END_LOCK;
        if (stopSweep) {
                DEBUG("Stopping Database reaper thread...\n");
                Sem_signal(P->alarm);
                Thread_join(P->reaper);
                Sem_destroy(P->alarm);
        }
}


Connection_T ConnectionPool_getConnection(T P) {
	Connection_T con = NULL;
	assert(P);
	LOCK(P->mutex) 
        {
                int i, size = Vector_size(P->pool);
                for (i = 0; i < size; i++) {
                        con = Vector_get(P->pool, i);
                        if (Connection_isAvailable(con) && Connection_ping(con)) {
                                Connection_setAvailable(con, false);
                                goto done;
                        } 
                }
                con = NULL;
                if (size < P->maxConnections) {
                        con = Connection_new(P, &P->error);
                        if (con) {
                                Connection_setAvailable(con, false);
                                Vector_push(P->pool, con);
                        } else {
                                DEBUG("Failed to create connection -- %s\n", P->error);
                                FREE(P->error);
                        }
                }
        }
done: 
        END_LOCK;
	return con;
}


void ConnectionPool_returnConnection(T P, Connection_T connection) {
	assert(P);
        assert(connection);
	if (Connection_isInTransaction(connection)) {
                TRY Connection_rollback(connection); ELSE END_TRY;
	}
	Connection_clear(connection);
	LOCK(P->mutex)
        {
		Connection_setAvailable(connection, true);
        }
	END_LOCK;
}


int ConnectionPool_reapConnections(T P) {
        int n = 0;
        assert(P);
        LOCK(P->mutex)
        {
                n = reapConnections(P);
        }
        END_LOCK;
        return n;
}


const char *ConnectionPool_version(void) {
        return ABOUT;
}
