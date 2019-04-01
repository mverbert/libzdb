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

#ifndef ORACLEADAPTER_INCLUDED
#define ORACLEADAPTER_INCLUDED

#include <oci.h>

#include "zdb.h"
#include "system/Time.h"

#define WATCHDOG(FUNCNAME, TYPENAME)      \
static void *FUNCNAME(void *args) {       \
    TYPENAME S = args;                    \
    while (S->svc) {                      \
        if (S->running) {                 \
            if (S->countdown <= 0) {      \
                OCIBreak(S->svc, S->err); \
                S->running = false;       \
            } else {                      \
                S->countdown -= 10;       \
            }                             \
        }                                 \
        Time_usleep(10000);               \
    }                                     \
    return NULL;                          \
}

const char *OraclePreparedStatement_getLastError(int err, OCIError *errhp) __attribute__ ((visibility("hidden")));

ResultSetDelegate_T OracleResultSet_new(Connection_T delegator, OCIStmt *stmt, OCIEnv *env, OCISession* usr, OCIError *err, OCISvcCtx *svc, int need_free) __attribute__ ((visibility("hidden")));
PreparedStatementDelegate_T OraclePreparedStatement_new(Connection_T delegator, OCIStmt *stmt, OCIEnv *env, OCISession* usr, OCIError *err, OCISvcCtx *svc) __attribute__ ((visibility("hidden")));

#endif
