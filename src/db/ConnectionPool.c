/*
 * Copyright (C) 2008 Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
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
#include "Vector.h"
#include "ResultSet.h"
#include "PreparedStatement.h"
#include "Connection.h"
#include "ConnectionPool.h"


/**
 * Implementation of the ConnectionPool interface
 *
 * @version \$Id: ConnectionPool.c,v 1.59 2008/03/20 11:28:53 hauk Exp $
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#define T ConnectionPool_T
struct T {
        URL_T url;
        int filled;
        int doSweep;
        char *error;
        int stopped;
        Sem_T alarm;
	Mutex_T mutex;
	Vector_T pool;
        Thread_T reaper;
        int sweepInterval;
	int maxConnections;
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


/* ------------------------------------------------------------ Prototypes */


static int fillPool(T P);
static int getActive(T P);
static void drainPool(T P);
static int reapConnections(T P);
static void *doSweep(void *args);
        

/* ---------------------------------------------------------------- Public */


T ConnectionPool_new(URL_T url) {
        T P;
	if (url==NULL)
                return NULL;
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
                P->initialConnections = connections;
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
                P->maxConnections = maxConnections;
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
                P->doSweep = true;
                P->sweepInterval = sweepInterval;
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
        n = getActive(P);
        END_LOCK;
        return n;
}


/* -------------------------------------------------------- Public methods */


void ConnectionPool_start(T P) {
        assert(P);
        LOCK(P->mutex)
                P->stopped = false;
                if (! P->filled) {
                        P->filled = fillPool(P);
                        if (P->filled && P->doSweep) {
                                DEBUG("Starting reaper thread\n");
                                Sem_init(P->alarm);
                                Thread_create(P->reaper, doSweep, P);
                        }
                }
        END_LOCK;
        if (! P->filled) {
               ABORT("Failed to start connection pool -- %s\n", P->error);
        }
}


void ConnectionPool_stop(T P) {
        int stopSweep = false;
        assert(P);
        LOCK(P->mutex)
                P->stopped = true;
                if (P->filled) {
                        drainPool(P);
                        P->filled = false;
                        stopSweep = (P->doSweep && P->reaper);
                }
        END_LOCK;
        if (stopSweep) {
                DEBUG("Stopping reaper thread...\n");
                Sem_signal(P->alarm);
                Thread_join(P->reaper);
                Sem_destroy(P->alarm);
        }
}


Connection_T ConnectionPool_getConnection(T P) {
	Connection_T con = NULL;
	assert(P);
	LOCK(P->mutex) 
	        int i, size = Vector_size(P->pool);
		for (i= 0; i < size; i++) {
			con = Vector_get(P->pool, i);
			if (Connection_isAvailable(con) && Connection_ping(con)) {
				Connection_setAvailable(con, false);
                                Connection_setQueryTimeout(con, SQL_DEFAULT_TIMEOUT);
				goto found;
			} 
		}
                if (size < P->maxConnections) {
                        con = Connection_new(P, &P->error);
                        if (con) {
                                Connection_setAvailable(con, false);
                                Vector_push(P->pool, con);
                                goto found;
                        } else {
                                DEBUG("Failed to create connection -- %s\n", P->error);
				FREE(P->error);
			}
                }
found:
        END_LOCK;
	return con;
}


void ConnectionPool_returnConnection(T P, Connection_T connection) {
	assert(P);
        assert(connection);
	if (Connection_isInTransaction(connection)) {
                TRY
                        Connection_rollback(connection);
                ELSE
                END_TRY;
	}
	Connection_clear(connection);
	LOCK(P->mutex)
		Connection_setAvailable(connection, true);
	END_LOCK;
}


int ConnectionPool_reapConnections(T P) {
        int n = 0;
        assert(P);
        LOCK(P->mutex)
                n = reapConnections(P);
        END_LOCK;
        return n;
}


const char *ConnectionPool_version() {
        return ABOUT;
}


/* ------------------------------------------------------- Private methods */


static void drainPool(T P) {
	Connection_T con;
        while (! Vector_isEmpty(P->pool)) {
		con = Vector_pop(P->pool);
		Connection_free(&con);
	}
        assert(Vector_isEmpty(P->pool));
}


static int fillPool(T P) {
	int i;
	Connection_T con;
        P->error = NULL;
	for (i = 0; i < P->initialConnections; i++) {
		if (! (con = Connection_new(P, &P->error))) {
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
        for (i = 0; i < size; i++) { 
                Connection_T con = Vector_get(P->pool, i);
                if (! Connection_isAvailable(con )) n++; 
        }
        return n; 
}


static int reapConnections(T P) {
        int i, x, n = 0;
        long timedout;
        Connection_T con;
        x = Vector_size(P->pool)-getActive(P)-P->initialConnections;
        timedout = Util_seconds()-P->connectionTimeout;
        while (x-->0) {
                for (i = 0; i < Vector_size(P->pool); i++) {
                        con = Vector_get(P->pool, i);
                        if (! Connection_isAvailable(con))
                                continue;
                        break;
                }
                if ((Connection_getLastAccessedTime(con) < timedout) || (! Connection_ping(con))) {
                        Vector_remove(P->pool, i);
                        Connection_free(&con);
                        n++;
                }
        }
        return n;
}


static void *doSweep(void *args) {
        T P = args;
        struct timespec wait = {0, 0};
        Mutex_lock(P->mutex);
        while (! P->stopped) {
                wait.tv_sec = Util_seconds() + P->sweepInterval;
                Sem_timeWait(P->alarm,  P->mutex, wait);
                if (P->stopped) break;
                reapConnections(P);
        }
        Mutex_unlock(P->mutex);
        DEBUG("Reaper thread stopped\n");
        return NULL;
}
