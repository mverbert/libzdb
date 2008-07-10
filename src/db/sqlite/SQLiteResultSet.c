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
#include <string.h>
#include <sqlite3.h>

#include "ResultSetStrategy.h"
#include "SQLiteResultSet.h"


/**
 * Implementation of the ResultSet/Strategy interface for SQLite. 
 * Accessing columns with index outside range throws SQLException
 *
 * @version \$Id: SQLiteResultSet.c,v 1.30 2008/03/20 11:28:54 hauk Exp $
 * @file
 */


/* ------------------------------------------------------------- Definitions */


const struct Rop_T sqlite3rops = {
	"sqlite",
        SQLiteResultSet_free,
        SQLiteResultSet_getColumnCount,
        SQLiteResultSet_getColumnName,
        SQLiteResultSet_next,
        SQLiteResultSet_getColumnSize,
        SQLiteResultSet_getString,
        SQLiteResultSet_getStringByName,
        SQLiteResultSet_getInt,
        SQLiteResultSet_getIntByName,
        SQLiteResultSet_getLLong,
        SQLiteResultSet_getLLongByName,
        SQLiteResultSet_getDouble,
        SQLiteResultSet_getDoubleByName,
        SQLiteResultSet_getBlob,
        SQLiteResultSet_getBlobByName,
        SQLiteResultSet_readData
};

#define T IResultSet_T
struct T {
        int keep;
        int maxRows;
	int currentRow;
	int columnCount;
	sqlite3_stmt *stmt;
};

#define TEST_INDEX(RETVAL) \
        int i; assert(R); i= columnIndex - 1; if (R->columnCount <= 0 || \
        i < 0 || i >= R->columnCount) { THROW(SQLException, "Column index out of range"); return(RETVAL); }
#define GET_INDEX(RETVAL) \
        int i; assert(R); if ((i= getIndex(R, columnName))<0) { \
        THROW(SQLException, "Invalid column name"); return (RETVAL); }


/* ------------------------------------------------------- Private methods */


static inline int getIndex(T R, const char *name) {
        int i;
        for (i = 0; i < R->columnCount; i++)
                if (Str_isByteEqual(name, sqlite3_column_name(R->stmt, i)))
                        return (i+1);
        return -1;
}


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


const char *SQLiteResultSet_getColumnName(T R, int column) {
	int i;
	assert(R);
	i = column - 1;
	if (R->columnCount <= 0 ||
	   i < 0                ||
	   i > R->columnCount)
                return NULL;
	return sqlite3_column_name(R->stmt, i);
}


int SQLiteResultSet_next(T R) {
        int status;
	assert(R);
        if (R->maxRows && (R->currentRow++ >= R->maxRows))
                return false;
        EXEC_SQLITE(status, sqlite3_step(R->stmt), SQL_DEFAULT_TIMEOUT);
	return (status == SQLITE_ROW);
}


long SQLiteResultSet_getColumnSize(T R, int columnIndex) {
        TEST_INDEX(-1)
        return sqlite3_column_bytes(R->stmt, i);
}


const char *SQLiteResultSet_getString(T R, int columnIndex) {
        TEST_INDEX(NULL)
	return sqlite3_column_text(R->stmt, i);
}


const char *SQLiteResultSet_getStringByName(T R, const char *columnName) {
        GET_INDEX(NULL)
        return SQLiteResultSet_getString(R, i);
}


int SQLiteResultSet_getInt(T R, int columnIndex) {
        TEST_INDEX(0)
	return sqlite3_column_int(R->stmt, i);
}


int SQLiteResultSet_getIntByName(T R, const char *columnName) {
        GET_INDEX(0)
        return SQLiteResultSet_getInt(R, i);
}


long long int SQLiteResultSet_getLLong(T R, int columnIndex) {
        TEST_INDEX(0)
	return (long long int)sqlite3_column_int64(R->stmt, i);
}


long long int SQLiteResultSet_getLLongByName(T R, const char *columnName) {
        GET_INDEX(0)
        return SQLiteResultSet_getLLong(R, i);
}


double SQLiteResultSet_getDouble(T R, int columnIndex) {
        TEST_INDEX(0.0)
	return sqlite3_column_double(R->stmt, i);
}


double SQLiteResultSet_getDoubleByName(T R, const char *columnName) {
        GET_INDEX(0.0)
        return SQLiteResultSet_getDouble(R, i);
}


const void *SQLiteResultSet_getBlob(T R, int columnIndex, int *size) {
        const void *blob;
        TEST_INDEX(NULL)
        blob = sqlite3_column_blob(R->stmt, i);
        *size = sqlite3_column_bytes(R->stmt, i);
        return (void*)blob;
}


const void *SQLiteResultSet_getBlobByName(T R, const char *columnName, int *size) {
        GET_INDEX(NULL)
        return SQLiteResultSet_getBlob(R, i, size);
}


int SQLiteResultSet_readData(T R, int columnIndex, void *b, int l, long off) {
        long r;
        int size;
        const void *blob;
        TEST_INDEX(0)
        blob = sqlite3_column_blob(R->stmt, i);
        size = sqlite3_column_bytes(R->stmt, i);
        if (off>size)
                return 0;
        r = off+l>size?size-off:l;
        memcpy(b, blob + off, r);
        return r;
}


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif

