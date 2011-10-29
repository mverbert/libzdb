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
#include <sqlite3.h>

#include "system/Time.h"
#include "ResultSet.h"
#include "SQLiteResultSet.h"
#include "PreparedStatementDelegate.h"
#include "SQLitePreparedStatement.h"


/**
 * Implementation of the PreparedStatement/Delegate interface for SQLite 
 *
 * @file
 */


/* ----------------------------------------------------------- Definitions */


const struct Pop_T sqlite3pops = {
        "sqlite",
        SQLitePreparedStatement_free,
        SQLitePreparedStatement_setString,
        SQLitePreparedStatement_setInt,
        SQLitePreparedStatement_setLLong,
        SQLitePreparedStatement_setDouble,
        SQLitePreparedStatement_setBlob,
        SQLitePreparedStatement_execute,
        SQLitePreparedStatement_executeQuery
};

#define T PreparedStatementDelegate_T
struct T {
        sqlite3 *db;
        int maxRows;
        int lastError;
	sqlite3_stmt *stmt;
};

extern const struct Rop_T sqlite3rops;


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T SQLitePreparedStatement_new(sqlite3 *db, void *stmt, int maxRows) {
        T P;
        assert(stmt);
        NEW(P);
        P->db = db;
        P->stmt = stmt;
        P->maxRows = maxRows;
        P->lastError = SQLITE_OK;
        return P;
}


void SQLitePreparedStatement_free(T *P) {
	assert(P && *P);
	sqlite3_finalize((*P)->stmt);
	FREE(*P);
}


void SQLitePreparedStatement_setString(T P, int parameterIndex, const char *x) {
        assert(P);
        sqlite3_reset(P->stmt);
        int size = x ? (int)strlen(x) : 0; 
        P->lastError = sqlite3_bind_text(P->stmt, parameterIndex, x, size, SQLITE_STATIC);
        if (P->lastError == SQLITE_RANGE)
                THROW(SQLException, "Parameter index is out of range");
}


void SQLitePreparedStatement_setInt(T P, int parameterIndex, int x) {
        assert(P);
        sqlite3_reset(P->stmt);
        P->lastError = sqlite3_bind_int(P->stmt, parameterIndex, x);
        if (P->lastError == SQLITE_RANGE)
                THROW(SQLException, "Parameter index is out of range");
}


void SQLitePreparedStatement_setLLong(T P, int parameterIndex, long long int x) {
        assert(P);
        sqlite3_reset(P->stmt);
        P->lastError = sqlite3_bind_int64(P->stmt, parameterIndex, x);
        if (P->lastError == SQLITE_RANGE)
                THROW(SQLException, "Parameter index is out of range");
}


void SQLitePreparedStatement_setDouble(T P, int parameterIndex, double x) {
        assert(P);
        sqlite3_reset(P->stmt);
        P->lastError = sqlite3_bind_double(P->stmt, parameterIndex, x);
        if (P->lastError == SQLITE_RANGE)
                THROW(SQLException, "Parameter index is out of range");
}


void SQLitePreparedStatement_setBlob(T P, int parameterIndex, const void *x, int size) {
        assert(P);
        sqlite3_reset(P->stmt);
        P->lastError = sqlite3_bind_blob(P->stmt, parameterIndex, x, size, SQLITE_STATIC);
        if (P->lastError == SQLITE_RANGE)
                THROW(SQLException, "Parameter index is out of range");
}


void SQLitePreparedStatement_execute(T P) {
        assert(P);
#if defined SQLITEUNLOCK && SQLITE_VERSION_NUMBER >= 3006012
        P->lastError = sqlite3_blocking_step(P->stmt);
#else
        EXEC_SQLITE(P->lastError, sqlite3_step(P->stmt), SQL_DEFAULT_TIMEOUT);
#endif
        switch (P->lastError)
        {
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


ResultSet_T SQLitePreparedStatement_executeQuery(T P) {
        assert(P);
        if (P->lastError == SQLITE_OK)
                return ResultSet_new(SQLiteResultSet_new(P->stmt, P->maxRows, true), (Rop_T)&sqlite3rops);
        THROW(SQLException, "%s", sqlite3_errmsg(P->db));
        return NULL;
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif
