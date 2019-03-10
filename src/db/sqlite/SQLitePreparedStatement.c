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

#include <stdio.h>
#include <time.h>
#include <string.h>

#include "SQLiteAdapter.h"


/**
 * Implementation of the PreparedStatement/Delegate interface for SQLite.
 * NOTE: SQLite starts parameter index at 1, so checkAndSetParameterIndex is not
 * needed
 *
 * @file
 */


/* ------------------------------------------------------------- Definitions */


#define T PreparedStatementDelegate_T
struct T {
        sqlite3 *db;
        int lastError;
        sqlite3_stmt *stmt;
        Connection_T delegator;
};
extern const struct Rop_T sqlite3rops;


/* ------------------------------------------------------------- Constructor */


T SQLitePreparedStatement_new(Connection_T delegator, sqlite3_stmt *stmt) {
        T P;
        assert(stmt);
        NEW(P);
        P->delegator = delegator;
        P->stmt = stmt;
        P->db = sqlite3_db_handle(stmt);
        P->lastError = SQLITE_OK;
        return P;
}


/* -------------------------------------------------------- Delegate Methods */


static void _free(T *P) {
        assert(P && *P);
        sqlite3_finalize((*P)->stmt);
        FREE(*P);
}


static void _setString(T P, int parameterIndex, const char *x) {
        assert(P);
        sqlite3_reset(P->stmt);
        int size = x ? (int)strlen(x) : 0;
        P->lastError = sqlite3_bind_text(P->stmt, parameterIndex, x, size, SQLITE_STATIC);
        if (P->lastError == SQLITE_RANGE)
                THROW(SQLException, "Parameter index is out of range");
}


static void _setInt(T P, int parameterIndex, int x) {
        assert(P);
        sqlite3_reset(P->stmt);
        P->lastError = sqlite3_bind_int(P->stmt, parameterIndex, x);
        if (P->lastError == SQLITE_RANGE)
                THROW(SQLException, "Parameter index is out of range");
}


static void _setLLong(T P, int parameterIndex, long long x) {
        assert(P);
        sqlite3_reset(P->stmt);
        P->lastError = sqlite3_bind_int64(P->stmt, parameterIndex, x);
        if (P->lastError == SQLITE_RANGE)
                THROW(SQLException, "Parameter index is out of range");
}


static void _setDouble(T P, int parameterIndex, double x) {
        assert(P);
        sqlite3_reset(P->stmt);
        P->lastError = sqlite3_bind_double(P->stmt, parameterIndex, x);
        if (P->lastError == SQLITE_RANGE)
                THROW(SQLException, "Parameter index is out of range");
}


static void _setTimestamp(T P, int parameterIndex, time_t x) {
        assert(P);
        sqlite3_reset(P->stmt);
        P->lastError = sqlite3_bind_int64(P->stmt, parameterIndex, x);
        if (P->lastError == SQLITE_RANGE)
                THROW(SQLException, "Parameter index is out of range");
}


static void _setBlob(T P, int parameterIndex, const void *x, int size) {
        assert(P);
        sqlite3_reset(P->stmt);
        P->lastError = sqlite3_bind_blob(P->stmt, parameterIndex, x, size, SQLITE_STATIC);
        if (P->lastError == SQLITE_RANGE)
                THROW(SQLException, "Parameter index is out of range");
}


static void _execute(T P) {
        assert(P);
        P->lastError = zdb_sqlite3_step(P->stmt);
        switch (P->lastError) {
                case SQLITE_DONE:
                        P->lastError = sqlite3_reset(P->stmt);
                        break;
                case SQLITE_ROW:
                        P->lastError = sqlite3_reset(P->stmt);
                        THROW(SQLException, "Select statement not allowed in PreparedStatement_execute()");
                        break;
                default:
                        P->lastError = sqlite3_reset(P->stmt);
                        THROW(SQLException, "%s", sqlite3_errmsg(P->db));
                        break;
        }
}


static ResultSet_T _executeQuery(T P) {
        assert(P);
        if (P->lastError == SQLITE_OK)
                return ResultSet_new(SQLiteResultSet_new(P->delegator, P->stmt, true), (Rop_T)&sqlite3rops);
        THROW(SQLException, "%s", sqlite3_errmsg(P->db));
        return NULL;
}


static long long _rowsChanged(T P) {
        assert(P);
        return (long long)sqlite3_changes(P->db);
}


static int _parameterCount(T P) {
        assert(P);
        return sqlite3_bind_parameter_count(P->stmt);
}


/* ------------------------------------------------------------------------- */


const struct Pop_T sqlite3pops = {
        .name           = "sqlite",
        .free           = _free,
        .setString      = _setString,
        .setInt         = _setInt,
        .setLLong       = _setLLong,
        .setDouble      = _setDouble,
        .setTimestamp   = _setTimestamp,
        .setBlob        = _setBlob,
        .execute        = _execute,
        .executeQuery   = _executeQuery,
        .rowsChanged    = _rowsChanged,
        .parameterCount = _parameterCount
};


