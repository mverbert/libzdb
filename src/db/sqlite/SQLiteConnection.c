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

#include "URL.h"
#include "ResultSet.h"
#include "StringBuffer.h"
#include "system/Time.h"
#include "PreparedStatement.h"
#include "SQLiteResultSet.h"
#include "SQLitePreparedStatement.h"
#include "ConnectionDelegate.h"
#include "SQLiteConnection.h"


/**
 * Implementation of the Connection/Delegate interface for SQLite 
 *
 * @file
 */


/* ----------------------------------------------------------- Definitions */


const struct Cop_T sqlite3cops = {
        "sqlite",
        SQLiteConnection_onstop,
        SQLiteConnection_new,
        SQLiteConnection_free,
        SQLiteConnection_setQueryTimeout,
        SQLiteConnection_setMaxRows,
        SQLiteConnection_ping,
        SQLiteConnection_beginTransaction,
        SQLiteConnection_commit,
        SQLiteConnection_rollback,
        SQLiteConnection_lastRowId,
        SQLiteConnection_rowsChanged,
        SQLiteConnection_execute,
        SQLiteConnection_executeQuery,
        SQLiteConnection_prepareStatement,
        SQLiteConnection_getLastError
};

#define T ConnectionDelegate_T
struct T {
        URL_T url;
	sqlite3 *db;
	int maxRows;
	int timeout;
	int lastError;
        StringBuffer_T sb;
};

extern const struct Rop_T sqlite3rops;
extern const struct Pop_T sqlite3pops;


/* ------------------------------------------------------- Private methods */


static sqlite3 *doConnect(URL_T url, char **error) {
        int status;
	sqlite3 *db;
        const char *path = URL_getPath(url);
        if (! path) {
                *error = Str_dup("no database specified in URL");
                return NULL;
        }
        /* Shared cache mode help reduce database lock problems if libzdb is used with many threads */
#if SQLITE_VERSION_NUMBER >= 3005000
        status = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_SHAREDCACHE, NULL);
#else
        status = sqlite3_open(path, &db);
#endif
        if (SQLITE_OK != status) {
                *error = Str_cat("cannot open database '%s' -- %s", path, sqlite3_errmsg(db));
                sqlite3_close(db);
                return NULL;
        }
	return db;
}


static inline void executeSQL(T C, const char *sql) {
#if defined SQLITEUNLOCK && SQLITE_VERSION_NUMBER >= 3006012
        C->lastError = sqlite3_blocking_exec(C->db, sql, NULL, NULL, NULL);
#else
        EXEC_SQLITE(C->lastError, sqlite3_exec(C->db, sql, NULL, NULL, NULL), C->timeout);
#endif
}


static int setProperties(T C, char **error) {
        const char **properties = URL_getParameterNames(C->url);
        if (properties) {
                StringBuffer_clear(C->sb);
                for (int i = 0; properties[i]; i++) {
                        if (IS(properties[i], "heap_limit")) // There is no PRAGMA for heap limit as of sqlite-3.7.0, so we make it a configurable property using "heap_limit" [kB]
                                #if defined(HAVE_SQLITE3_SOFT_HEAP_LIMIT64)
                                sqlite3_soft_heap_limit64(Str_parseInt(URL_getParameter(C->url, properties[i])) * 1024);
                                #elif defined(HAVE_SQLITE3_SOFT_HEAP_LIMIT)
                                sqlite3_soft_heap_limit(Str_parseInt(URL_getParameter(C->url, properties[i])) * 1024);
                                #else
                                DEBUG("heap_limit not supported by your sqlite3 version, please consider upgrading sqlite3\n");
                                #endif
                        else
                                StringBuffer_append(C->sb, "PRAGMA %s = %s; ", properties[i], URL_getParameter(C->url, properties[i]));
                }
                executeSQL(C, StringBuffer_toString(C->sb));
                if (C->lastError != SQLITE_OK) {
                        *error = Str_cat("unable to set database pragmas -- %s", sqlite3_errmsg(C->db));
                        return false;
                }
        }
        return true;
}


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T SQLiteConnection_new(URL_T url, char **error) {
	T C;
        sqlite3 *db;
	assert(url);
        assert(error);
        if (! (db = doConnect(url, error)))
                return NULL;
	NEW(C);
        C->db = db;
        C->url = url;
        C->timeout = SQL_DEFAULT_TIMEOUT;
        C->sb = StringBuffer_create(STRLEN);
        if (! setProperties(C, error))
                SQLiteConnection_free(&C);
	return C;
}


void SQLiteConnection_free(T *C) {
	assert(C && *C);
        while (sqlite3_close((*C)->db) == SQLITE_BUSY)
               Time_usleep(10);
        StringBuffer_free(&(*C)->sb);
	FREE(*C);
}


void SQLiteConnection_setQueryTimeout(T C, int ms) {
	assert(C);
        C->timeout = ms;
	sqlite3_busy_timeout(C->db, C->timeout);
}


void SQLiteConnection_setMaxRows(T C, int max) {
	assert(C);
	C->maxRows = max;
}


int SQLiteConnection_ping(T C) {
        assert(C);
        executeSQL(C, "select 1;");
        return (C->lastError == SQLITE_OK);
}


int SQLiteConnection_beginTransaction(T C) {
	assert(C);
        executeSQL(C, "BEGIN TRANSACTION;");
        return (C->lastError == SQLITE_OK);
}


int SQLiteConnection_commit(T C) {
	assert(C);
        executeSQL(C, "COMMIT TRANSACTION;");
        return (C->lastError == SQLITE_OK);
}


int SQLiteConnection_rollback(T C) {
	assert(C);
        executeSQL(C, "ROLLBACK TRANSACTION;");
        return (C->lastError == SQLITE_OK);
}


long long int SQLiteConnection_lastRowId(T C) {
        assert(C);
        return sqlite3_last_insert_rowid(C->db);
}


long long int SQLiteConnection_rowsChanged(T C) {
        assert(C);
        return (long long int)sqlite3_changes(C->db);
}


int SQLiteConnection_execute(T C, const char *sql, va_list ap) {
        va_list ap_copy;
	assert(C);
        StringBuffer_clear(C->sb);
        va_copy(ap_copy, ap);
        StringBuffer_vappend(C->sb, sql, ap_copy);
        va_end(ap_copy);
	executeSQL(C, StringBuffer_toString(C->sb));
	return (C->lastError == SQLITE_OK);
}


ResultSet_T SQLiteConnection_executeQuery(T C, const char *sql, va_list ap) {
        va_list ap_copy;
        const char *tail;
	sqlite3_stmt *stmt;
	assert(C);
        StringBuffer_clear(C->sb);
        va_copy(ap_copy, ap);
        StringBuffer_vappend(C->sb, sql, ap_copy);
        va_end(ap_copy);
#if defined SQLITEUNLOCK && SQLITE_VERSION_NUMBER >= 3006012
        C->lastError = sqlite3_blocking_prepare_v2(C->db, StringBuffer_toString(C->sb), StringBuffer_length(C->sb), &stmt, &tail);
#elif SQLITE_VERSION_NUMBER >= 3004000
        EXEC_SQLITE(C->lastError, sqlite3_prepare_v2(C->db, StringBuffer_toString(C->sb), StringBuffer_length(C->sb), &stmt, &tail), C->timeout);
#else
        EXEC_SQLITE(C->lastError, sqlite3_prepare(C->db, StringBuffer_toString(C->sb), StringBuffer_length(C->sb), &stmt, &tail), C->timeout);
#endif
	if (C->lastError == SQLITE_OK)
		return ResultSet_new(SQLiteResultSet_new(stmt, C->maxRows, false), (Rop_T)&sqlite3rops);
	return NULL;
}


PreparedStatement_T SQLiteConnection_prepareStatement(T C, const char *sql, va_list ap) {
        va_list ap_copy;
        const char *tail;
        sqlite3_stmt *stmt;
        assert(C);
        StringBuffer_clear(C->sb);
        va_copy(ap_copy, ap);
        StringBuffer_vappend(C->sb, sql, ap_copy);
        va_end(ap_copy);
#if defined SQLITEUNLOCK && SQLITE_VERSION_NUMBER >= 3006012
        C->lastError = sqlite3_blocking_prepare_v2(C->db, StringBuffer_toString(C->sb), -1, &stmt, &tail);
#elif SQLITE_VERSION_NUMBER >= 3004000
        EXEC_SQLITE(C->lastError, sqlite3_prepare_v2(C->db, StringBuffer_toString(C->sb), -1, &stmt, &tail), C->timeout);
#else
        EXEC_SQLITE(C->lastError, sqlite3_prepare(C->db, StringBuffer_toString(C->sb), -1, &stmt, &tail), C->timeout);
#endif
        if (C->lastError == SQLITE_OK)
		return PreparedStatement_new(SQLitePreparedStatement_new(C->db, stmt, C->maxRows), (Pop_T)&sqlite3pops);
	return NULL;
}


const char *SQLiteConnection_getLastError(T C) {
	assert(C);
	return sqlite3_errmsg(C->db);
}


/* Class Method: SQLite3 client library finalization */
void SQLiteConnection_onstop(void) {
#if SQLITE_VERSION_NUMBER >= 3006000
        sqlite3_shutdown();
#endif
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif
