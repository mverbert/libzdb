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

#include "system/Time.h"
#include "SQLiteAdapter.h"


/**
 * Implementation of the ResultSet/Delegate interface for SQLite.
 * Accessing columns with index outside range throws SQLException
 *
 * @file
 */


/* ------------------------------------------------------------- Definitions */


#define T ResultSetDelegate_T
struct T {
        sqlite3 *db;
        int keep;
        int maxRows;
        int lastError;
        int currentRow;
        int columnCount;
        sqlite3_stmt *stmt;
        Connection_T delegator;
};


/* ------------------------------------------------------------- Constructor */


T SQLiteResultSet_new(Connection_T delegator, sqlite3_stmt *stmt, int keep) {
        T R;
        assert(stmt);
        NEW(R);
        R->delegator = delegator;
        R->stmt = stmt;
        R->db = sqlite3_db_handle(stmt);
        R->keep = keep;
        R->maxRows = Connection_getMaxRows(delegator);
        R->columnCount = sqlite3_column_count(R->stmt);
        return R;
}


/* -------------------------------------------------------- Delegate Methods */


static void _free(T *R) {
        assert(R && *R);
        if ((*R)->keep)
                sqlite3_reset((*R)->stmt);
        else
                sqlite3_finalize((*R)->stmt);
        FREE(*R);
}


static int _getColumnCount(T R) {
        assert(R);
        return R->columnCount;
}


static const char *_getColumnName(T R, int columnIndex) {
        assert(R);
        columnIndex--;
        if (R->columnCount <= 0 || columnIndex < 0 || columnIndex > R->columnCount)
                return NULL;
        return sqlite3_column_name(R->stmt, columnIndex);
}


static long _getColumnSize(T R, int columnIndex) {
        int i = checkAndSetColumnIndex(columnIndex, R->columnCount);
        return sqlite3_column_bytes(R->stmt, i);
}


static bool _next(T R) {
        assert(R);
        if (R->maxRows && (R->currentRow++ >= R->maxRows))
                return false;
        R->lastError = zdb_sqlite3_step(R->stmt);
        if (R->lastError != SQLITE_ROW && R->lastError != SQLITE_DONE) {
#ifdef HAVE_SQLITE3_ERRSTR
                THROW(SQLException, "sqlite3_step -- %s", sqlite3_errstr(R->lastError));
#else
                THROW(SQLException, "sqlite3_step -- error code: %d", R->lastError);
#endif
        }
        return (R->lastError == SQLITE_ROW);
}


static bool _isnull(T R, int columnIndex) {
        assert(R);
        int i = checkAndSetColumnIndex(columnIndex, R->columnCount);
        return (sqlite3_column_type(R->stmt, i) == SQLITE_NULL);
}


static const char *_getString(T R, int columnIndex) {
        assert(R);
        int i = checkAndSetColumnIndex(columnIndex, R->columnCount);
        return (const char*)sqlite3_column_text(R->stmt, i);
}


static const void *_getBlob(T R, int columnIndex, int *size) {
        assert(R);
        int i = checkAndSetColumnIndex(columnIndex, R->columnCount);
        const void *blob = sqlite3_column_blob(R->stmt, i);
        *size = sqlite3_column_bytes(R->stmt, i);
        return blob;
}


static time_t _getTimestamp(T R, int columnIndex) {
        assert(R);
        int i = checkAndSetColumnIndex(columnIndex, R->columnCount);
        if (sqlite3_column_type(R->stmt, i) == SQLITE_INTEGER)
                return (time_t)sqlite3_column_int64(R->stmt, i);
        // Not an integer storage class, try parse as time string
        return Time_toTimestamp(sqlite3_column_text(R->stmt, i));
}


static struct tm *_getDateTime(T R, int columnIndex, struct tm *tm) {
        assert(R);
        int i = checkAndSetColumnIndex(columnIndex, R->columnCount);
        if (sqlite3_column_type(R->stmt, i) == SQLITE_INTEGER) {
                time_t utc = (time_t)sqlite3_column_int64(R->stmt, i);
                if (gmtime_r(&utc, tm)) tm->tm_year += 1900; // Use year literal
        } else {
                // Not an integer storage class, try parse as time string
                Time_toDateTime(sqlite3_column_text(R->stmt, i), tm);
        }
        return tm;
}


/* ------------------------------------------------------------------------- */


const struct Rop_T sqlite3rops = {
        .name           = "sqlite",
        .free           = _free,
        .getColumnCount = _getColumnCount,
        .getColumnName  = _getColumnName,
        .getColumnSize  = _getColumnSize,
        .next           = _next,
        .isnull         = _isnull,
        .getString      = _getString,
        .getBlob        = _getBlob,
        .getTimestamp   = _getTimestamp,
        .getDateTime    = _getDateTime
        // get/setFetchSize is not applicable for SQLite
};

