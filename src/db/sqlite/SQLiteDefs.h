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
#ifndef SQLITERESULTSET_INCLUDED
#define SQLITERESULTSET_INCLUDED
#include <stdlib.h>
#include <string.h>
#include "Thread.h"
#include "system/Time.h"

#include "StringBuffer.h"


#if defined SQLITEUNLOCK && SQLITE_VERSION_NUMBER >= 3006012
/* SQLite unlock notify based synchronization */
typedef struct UnlockNotification {
        int fired;
        Sem_T cond;
        Mutex_T mutex;
} UnlockNotification_T;

static inline void unlock_notify_cb(void **apArg, int nArg) {
        for (int i = 0; i < nArg; i++) {
                UnlockNotification_T *p = (UnlockNotification_T *)apArg[i];
                Mutex_lock(p->mutex);
                p->fired = 1;
                Sem_signal(p->cond);
                Mutex_unlock(p->mutex);
        }
}

static inline int wait_for_unlock_notify(sqlite3 *db){
        UnlockNotification_T un;
        un.fired = 0;
        Mutex_init(un.mutex);
        Sem_init(un.cond);
        int rc = sqlite3_unlock_notify(db, unlock_notify_cb, (void *)&un);
        assert(rc == SQLITE_LOCKED || rc == SQLITE_OK);
        if (rc == SQLITE_OK) {
                Mutex_lock(un.mutex);
                if (! un.fired)
                        Sem_wait(un.cond, un.mutex);
                Mutex_unlock(un.mutex);
        }
        Sem_destroy(un.cond);
        Mutex_destroy(un.mutex);
        return rc;
}

static inline int sqlite3_blocking_step(sqlite3_stmt *pStmt) {
        int rc;
        while (SQLITE_LOCKED == (rc = sqlite3_step(pStmt))) {
                rc = wait_for_unlock_notify(sqlite3_db_handle(pStmt));
                if (rc != SQLITE_OK)
                        break;
#if SQLITE_VERSION_NUMBER < 3070000 || defined SQLITE_OMIT_AUTORESET
                sqlite3_reset(pStmt);
#endif
        }
        return rc;
}

static inline int sqlite3_blocking_prepare_v2(sqlite3 *db, const char *zSql, int nSql, sqlite3_stmt **ppStmt, const char **pz) {
        int rc;
        while (SQLITE_LOCKED == (rc = sqlite3_prepare_v2(db, zSql, nSql, ppStmt, pz))) {
                rc = wait_for_unlock_notify(db);
                if (rc != SQLITE_OK)
                        break;
        }
        return rc;
}

static inline int sqlite3_blocking_exec(sqlite3 *db, const char *zSql, int (*callback)(void *, int, char **, char **), void *arg, char **errmsg) {
        int rc;
        while (SQLITE_LOCKED == (rc = sqlite3_exec(db, zSql, callback, arg, errmsg))) {
                rc = wait_for_unlock_notify(db);
                if (rc != SQLITE_OK)
                        break;
        }
        return rc;
}

static inline void _executeSQL(ConnectionDelegate_T C, const char *sql) {
        C->lastError = sqlite3_blocking_exec(C->db, sql, NULL, NULL, NULL);
}

#else
/* SQLite timed retry macro */
#define EXEC_SQLITE(T, F) do { \
        int steps = 10; long sleep = Connection_getQueryTimeout(T->delegator) * USEC_PER_MSEC / steps; \
        for (int i = 0; i < steps; i++) { T->lastError = (F); \
        if ((T->lastError != SQLITE_BUSY) && (T->lastError != SQLITE_LOCKED)) break; \
        Time_usleep(sleep); sleep += 97; }} while(0)

static inline void _executeSQL(T C, const char *sql) {
        EXEC_SQLITE(C, sqlite3_exec(C->db, sql, NULL, NULL, NULL));
}

#endif

#endif
