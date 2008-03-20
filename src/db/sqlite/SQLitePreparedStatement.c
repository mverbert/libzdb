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
#include <sqlite3.h>

#include "Mem.h"
#include "Str.h"
#include "Util.h"
#include "ResultSet.h"
#include "SQLiteResultSet.h"
#include "PreparedStatementStrategy.h"
#include "SQLitePreparedStatement.h"


/**
 * Implementation of the PreparedStatement/Strategy interface for SQLite 
 *
 * @version \$Id: SQLitePreparedStatement.c,v 1.23 2008/03/20 11:28:54 hauk Exp $
 * @file
 */


/* ----------------------------------------------------------- Definitions */


const struct prepop sqlite3prepops = {
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

#define T IPreparedStatement_T
struct T {
        int maxRows;
        int lastError;
	sqlite3_stmt *stmt;
};

extern const struct rsetop sqlite3rsetops;


/* ----------------------------------------------------- Protected methods */

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T SQLitePreparedStatement_new(void *stmt, int maxRows) {
        T P;
        assert(stmt);
        NEW(P);
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


int SQLitePreparedStatement_setString(T P, int parameterIndex, const char *x) {
        assert(P);
        sqlite3_reset(P->stmt);
        P->lastError = sqlite3_bind_text(P->stmt, parameterIndex, x, 
                                        -1, SQLITE_STATIC);
        return (P->lastError==SQLITE_OK);
}


int SQLitePreparedStatement_setInt(T P, int parameterIndex, int x) {
        assert(P);
        sqlite3_reset(P->stmt);
        P->lastError = sqlite3_bind_int(P->stmt, parameterIndex, x);
        return (P->lastError==SQLITE_OK);
}


int SQLitePreparedStatement_setLLong(T P, int parameterIndex, long long int x) {
        assert(P);
        sqlite3_reset(P->stmt);
        P->lastError = sqlite3_bind_int64(P->stmt, parameterIndex, x);
        return (P->lastError==SQLITE_OK);
}


int SQLitePreparedStatement_setDouble(T P, int parameterIndex, double x) {
        assert(P);
        sqlite3_reset(P->stmt);
        P->lastError = sqlite3_bind_double(P->stmt, parameterIndex, x);
        return (P->lastError==SQLITE_OK);
}


int SQLitePreparedStatement_setBlob(T P, int parameterIndex, const void *x, 
                                    int size) {
        assert(P);
        sqlite3_reset(P->stmt);
        P->lastError = sqlite3_bind_blob(P->stmt, parameterIndex, x, 
                                         size, SQLITE_STATIC);
        return (P->lastError==SQLITE_OK);
}


int SQLitePreparedStatement_execute(T P) {
        assert(P);
        EXEC_SQLITE(P->lastError, sqlite3_step(P->stmt), SQL_DEFAULT_TIMEOUT);
        if (P->lastError==SQLITE_DONE) {
                P->lastError = sqlite3_reset(P->stmt);
                return (P->lastError==SQLITE_OK);
        }
        if (P->lastError==SQLITE_ROW) {
                THROW(SQLException, "Select statement not allowed in PreparedStatement_execute()");
                P->lastError = sqlite3_reset(P->stmt);
        }
        return false;
}


ResultSet_T SQLitePreparedStatement_executeQuery(T P) {
        assert(P);
        if (P->lastError==SQLITE_OK) {
                return ResultSet_new(SQLiteResultSet_new(P->stmt, P->maxRows, true), (Rop_T)&sqlite3rsetops);
        }
        return NULL;
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif
