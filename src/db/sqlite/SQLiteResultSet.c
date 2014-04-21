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
#include <string.h>
#include <sqlite3.h>

#include "system/Time.h"
#include "ResultSetDelegate.h"
#include "SQLiteResultSet.h"


/**
 * Implementation of the ResultSet/Delegate interface for SQLite. 
 * Accessing columns with index outside range throws SQLException
 *
 * @file
 */


/* ------------------------------------------------------------- Definitions */


const struct Rop_T sqlite3rops = {
	.name           = "sqlite",
        .free           = SQLiteResultSet_free,
        .getColumnCount = SQLiteResultSet_getColumnCount,
        .getColumnName  = SQLiteResultSet_getColumnName,
        .getColumnSize  = SQLiteResultSet_getColumnSize,
        .next           = SQLiteResultSet_next,
        .isnull         = SQLiteResultSet_isnull,
        .getString      = SQLiteResultSet_getString,
        .getBlob        = SQLiteResultSet_getBlob,
        .getTimestamp   = SQLiteResultSet_getTimestamp,
        .getDateTime    = SQLiteResultSet_getDateTime
};

#define T ResultSetDelegate_T
struct T {
        int keep;
        int maxRows;
	int currentRow;
	int columnCount;
	sqlite3_stmt *stmt;
};


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T SQLiteResultSet_new(void *stmt, int maxRows, int keep) {
	T R;
	assert(stmt);
	NEW(R);
	R->stmt = stmt;
        R->keep = keep;
        R->maxRows = maxRows;
        R->columnCount = sqlite3_column_count(R->stmt);
	return R;
}


void SQLiteResultSet_free(T *R) {
	assert(R && *R);
        if ((*R)->keep)
                sqlite3_reset((*R)->stmt);
        else
                sqlite3_finalize((*R)->stmt);
	FREE(*R);
}


int SQLiteResultSet_getColumnCount(T R) {
	assert(R);
	return R->columnCount;
}


const char *SQLiteResultSet_getColumnName(T R, int columnIndex) {
	assert(R);
	columnIndex--;
	if (R->columnCount <= 0 || columnIndex < 0 || columnIndex > R->columnCount)
                return NULL;
	return sqlite3_column_name(R->stmt, columnIndex);
}


long SQLiteResultSet_getColumnSize(T R, int columnIndex) {
        int i = checkAndSetColumnIndex(columnIndex, R->columnCount);
        return sqlite3_column_bytes(R->stmt, i);
}


int SQLiteResultSet_next(T R) {
        int status;
	assert(R);
        if (R->maxRows && (R->currentRow++ >= R->maxRows))
                return false;
#if defined SQLITEUNLOCK && SQLITE_VERSION_NUMBER >= 3006012
	status = sqlite3_blocking_step(R->stmt);
#else
        EXEC_SQLITE(status, sqlite3_step(R->stmt), SQL_DEFAULT_TIMEOUT);
#endif
        if (status != SQLITE_ROW && status != SQLITE_DONE) {
#ifdef HAVE_SQLITE3_ERRSTR
                THROW(SQLException, "sqlite3_step -- %s", sqlite3_errstr(status));
#else
                THROW(SQLException, "sqlite3_step -- error code: %d", status);
#endif
        }
        return (status == SQLITE_ROW);
}


int SQLiteResultSet_isnull(T R, int columnIndex) {
        assert(R);
        int i = checkAndSetColumnIndex(columnIndex, R->columnCount);
        return (sqlite3_column_type(R->stmt, i) == SQLITE_NULL);
}


const char *SQLiteResultSet_getString(T R, int columnIndex) {
        assert(R);
        int i = checkAndSetColumnIndex(columnIndex, R->columnCount);
	return (const char*)sqlite3_column_text(R->stmt, i);
}


const void *SQLiteResultSet_getBlob(T R, int columnIndex, int *size) {
        assert(R);
        int i = checkAndSetColumnIndex(columnIndex, R->columnCount);
        const void *blob = sqlite3_column_blob(R->stmt, i);
        *size = sqlite3_column_bytes(R->stmt, i);
        return blob;
}


time_t SQLiteResultSet_getTimestamp(T R, int columnIndex) {
        assert(R);
        int i = checkAndSetColumnIndex(columnIndex, R->columnCount);
        if (sqlite3_column_type(R->stmt, i) == SQLITE_INTEGER)
                return (time_t)sqlite3_column_int64(R->stmt, i);
        // Not integer storage class, try to parse as time string
        return Time_toTimestamp(sqlite3_column_text(R->stmt, i));
}


struct tm *SQLiteResultSet_getDateTime(T R, int columnIndex, struct tm *tm) {
        assert(R);
        int i = checkAndSetColumnIndex(columnIndex, R->columnCount);
        if (sqlite3_column_type(R->stmt, i) == SQLITE_INTEGER) {
                time_t utc = (time_t)sqlite3_column_int64(R->stmt, i);
                if (gmtime_r(&utc, tm)) tm->tm_year += 1900; // Use year literal 
        } else {
                // Not integer storage class, try to parse as time string
                Time_toDateTime(sqlite3_column_text(R->stmt, i), tm);
        }
        return tm;
}


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif

