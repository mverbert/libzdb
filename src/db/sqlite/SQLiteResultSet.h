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
#ifndef SQLITERESULTSET_INCLUDED
#define SQLITERESULTSET_INCLUDED
#include <stdlib.h>
#include <string.h>
#include "Thread.h"


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
                sqlite3_reset(pStmt);
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
#else
/* SQLite timed retry macro */
#define EXEC_SQLITE(status, action, timeout) \
        do {\
                long t = (timeout * USEC_PER_MSEC);\
                int x = 0;\
                do {\
                        status = (action);\
                } while (((status == SQLITE_BUSY) || (status == SQLITE_LOCKED)) && (x++ <= 9) && ((Time_usleep(t/(rand() % 10 + 100)))));\
        } while (0)
#endif


#define T ResultSetDelegate_T
T SQLiteResultSet_new(void *stmt, int maxRows, int keep);
void SQLiteResultSet_free(T *R);
int SQLiteResultSet_getColumnCount(T R);
const char *SQLiteResultSet_getColumnName(T R, int column);
int SQLiteResultSet_next(T R);
long SQLiteResultSet_getColumnSize(T R, int columnIndex);
const char *SQLiteResultSet_getString(T R, int columnIndex);
const void *SQLiteResultSet_getBlob(T R, int columnIndex, int *size);
#undef T
#endif
