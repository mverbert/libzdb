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

#include "Config.h"

#include <stdlib.h>
#include "Thread.h"
#include "system/Time.h"

#include "SQLiteDefs.h"


#if defined SQLITEUNLOCK && SQLITE_VERSION_NUMBER >= 3006012

/*
 * SQLite unlock notify based synchronization
 */

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


static inline int sqlite3_blocking_exec(sqlite3 *db, const char *zSql, int (*callback)(void *, int, char **, char **), void *arg, char **errmsg) {
        int rc;
        while (SQLITE_LOCKED == (rc = sqlite3_exec(db, zSql, callback, arg, errmsg))) {
                rc = wait_for_unlock_notify(db);
                if (rc != SQLITE_OK)
                        break;
        }
        return rc;
}


// MARK: - Blocking API

int zdb_sqlite3_step(sqlite3_stmt *pStmt) {
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


int zdb_sqlite3_prepare_v2(sqlite3 *db, const char *zSql, int nSql, sqlite3_stmt **ppStmt, const char **pz) {
        int rc;
        while (SQLITE_LOCKED == (rc = sqlite3_prepare_v2(db, zSql, nSql, ppStmt, pz))) {
                rc = wait_for_unlock_notify(db);
                if (rc != SQLITE_OK)
                        break;
        }
        return rc;
}


int zdb_sqlite3_exec(sqlite3 *db, const char *sql) {
        return sqlite3_blocking_exec(db, sql, NULL, NULL, NULL);
}


#else

/*
 * SQLite timed retry
 */


// Exponential backoff https://en.wikipedia.org/wiki/Exponential_backoff
static inline void _backoff(int step) {
#define slot 51
        switch (step) {
                case 0:
                        Time_usleep(slot * (random() % 2));
                        break;
                case 1:
                        Time_usleep(slot * (random() % 4));
                        break;
                default:
                        // 51 µs * R[0...2^step - 1]
                        Time_usleep(slot * (random() % (1 << step)));
                        break;
        }
}


int zdb_sqlite3_exec(sqlite3 *db, const char *sql) {
        for (int i = 0, steps = 10; i < steps; i++) {
                int status = sqlite3_exec(db, sql, NULL, NULL, NULL);
                if ((status != SQLITE_BUSY) && (status != SQLITE_LOCKED))
                        return status;
                _backoff(i);
        }
        return sqlite3_exec(db, sql, NULL, NULL, NULL);
}

#endif
