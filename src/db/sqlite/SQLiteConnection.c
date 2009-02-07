/*
 * Copyright (C) 2004-2009 Tildeslash Ltd. All rights reserved.
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
#include "PreparedStatement.h"
#include "SQLiteResultSet.h"
#include "SQLitePreparedStatement.h"
#include "ConnectionStrategy.h"
#include "SQLiteConnection.h"


/**
 * Implementation of the Connection/Strategy interface for SQLite 
 *
 * @version \$Id: SQLiteConnection.c,v 1.36 2008/03/20 11:28:54 hauk Exp $
 * @file
 */


/* ----------------------------------------------------------- Definitions */


const struct Cop_T sqlite3cops = {
        "sqlite",
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

#define T ConnectionImpl_T
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


/* ------------------------------------------------------------ Prototypes */


static int setProperties(T C, char **error);
static sqlite3 *doConnect(URL_T url, char **error);
static inline void executeSQL(T C, const char *sql);


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif


T SQLiteConnection_new(URL_T url, char **error) {
	T C;
        sqlite3 *db;
	assert(url);
        assert(error);
        if ((db = doConnect(url, error))==NULL)
                return NULL;
	NEW(C);
        C->db = db;
        C->url = url;
        C->sb = StringBuffer_new("");
        if (! setProperties(C, error)) {
                SQLiteConnection_free(&C);
                return NULL;
        }
        C->timeout = SQL_DEFAULT_TIMEOUT;
	return C;
}


void SQLiteConnection_free(T *C) {
	assert(C && *C);
        while ((sqlite3_close((*C)->db)==SQLITE_BUSY) && Util_usleep(1000));
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
        return (C->lastError==SQLITE_OK);
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
	if (C->lastError == SQLITE_OK)
                return true;
        else {
                THROW(SQLException, "SQLiteConnection_execute -- %s", sqlite3_errmsg(C->db));
                return false;
        }
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
#if SQLITE_VERSION_NUMBER >= 3004000
        EXEC_SQLITE(C->lastError, sqlite3_prepare_v2(C->db, StringBuffer_toString(C->sb), StringBuffer_length(C->sb), &stmt, &tail), C->timeout);
#else
        EXEC_SQLITE(C->lastError, sqlite3_prepare(C->db, StringBuffer_toString(C->sb), StringBuffer_length(C->sb), &stmt, &tail), C->timeout);
#endif
	if (C->lastError==SQLITE_OK)
		return ResultSet_new(SQLiteResultSet_new(stmt, C->maxRows, false), (Rop_T)&sqlite3rops);
        THROW(SQLException, "SQLiteConnection_executeQuery -- %s", sqlite3_errmsg(C->db));
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
#if SQLITE_VERSION_NUMBER >= 3004000
        EXEC_SQLITE(C->lastError, sqlite3_prepare_v2(C->db, StringBuffer_toString(C->sb), -1, &stmt, &tail), C->timeout);
#else
        EXEC_SQLITE(C->lastError, sqlite3_prepare(C->db, StringBuffer_toString(C->sb), -1, &stmt, &tail), C->timeout);
#endif
        if (C->lastError==SQLITE_OK)
		return PreparedStatement_new(SQLitePreparedStatement_new(C->db, stmt, C->maxRows), (Pop_T)&sqlite3pops);
        THROW(SQLException, "SQLiteConnection_prepareStatement -- %s", sqlite3_errmsg(C->db));
	return NULL;
}


const char *SQLiteConnection_getLastError(T C) {
	assert(C);
	return sqlite3_errmsg(C->db);
}


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif


/* ------------------------------------------------------- Private methods */


static sqlite3 *doConnect(URL_T url, char **error) {
	sqlite3 *db;
        const char *path = URL_getPath(url);
        if (path[0] == '/' && path[1] == ':') {
                if (IS(path, "/:memory:")==false) {
                        *error = Str_cat("unknown database '%s', did you mean '/:memory:'?", path);
                        return NULL;
                }
                path++;
        }
        if (SQLITE_OK != sqlite3_open(path, &db)) {
                *error = Str_cat("cannot open database '%s' -- %s", path, sqlite3_errmsg(db));
                sqlite3_close(db);
                return NULL;
        }
	return db;
}


static int setProperties(T C, char **error) {
        int i;
        const char **properties = URL_getParameterNames(C->url);
        if (properties) {
                StringBuffer_clear(C->sb);
                for (i = 0; properties[i]; i++)
                        StringBuffer_append(C->sb, "PRAGMA %s = %s; ", properties[i], URL_getParameter(C->url, properties[i]));
                executeSQL(C, StringBuffer_toString(C->sb));
                if (C->lastError!=SQLITE_OK) {
                        *error = Str_cat("unable to set database pragmas -- %s", sqlite3_errmsg(C->db));
                        return false;
                }
        }
        return true;
}


static inline void executeSQL(T C, const char *sql) {
        EXEC_SQLITE(C->lastError, sqlite3_exec(C->db, sql, NULL, NULL, NULL), C->timeout);
}
