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
#include <mysql.h>
#include <errmsg.h>

#include "ResultSetStrategy.h"
#include "MysqlResultSet.h"


/**
 * Implementation of the ResultSet/Strategy interface for mysql. 
 * Accessing columns with index outside range throws SQLException
 *
 * @version \$Id: MysqlResultSet.c,v 1.51 2008/03/20 11:28:53 hauk Exp $
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#define MYSQL_OK 0

const struct rop mysqlrops = {
	"mysql",
        MysqlResultSet_free,
        MysqlResultSet_getColumnCount,
        MysqlResultSet_getColumnName,
        MysqlResultSet_next,
        MysqlResultSet_getColumnSize,
        MysqlResultSet_getString,
        MysqlResultSet_getStringByName,
        MysqlResultSet_getInt,
        MysqlResultSet_getIntByName,
        MysqlResultSet_getLLong,
        MysqlResultSet_getLLongByName,
        MysqlResultSet_getDouble,
        MysqlResultSet_getDoubleByName,
        MysqlResultSet_getBlob,
        MysqlResultSet_getBlobByName,
        MysqlResultSet_readData
};

typedef struct column_t {
        my_bool is_null;
        MYSQL_FIELD *field;
        unsigned long real_length;
        char *buffer;
} *column_t;

#define T IResultSet_T
struct T {
        int stop;
        int keep;
        int maxRows;
        int lastError;
	int currentRow;
	int columnCount;
        MYSQL_RES *meta;
        MYSQL_BIND *bind;
	MYSQL_STMT *stmt;
        column_t columns;
};

#define TEST_INDEX(RETVAL) \
        int i; assert(R);i= columnIndex-1; if (R->columnCount <= 0 || \
        i < 0 || i >= R->columnCount) { THROW(SQLException, "Column index out of range"); \
        return(RETVAL); } if (R->columns[i].is_null) return (RETVAL); 
#define GET_INDEX(RETVAL) \
        int i;assert(R);if ((i=getIndex(R,columnName))<0) { \
        THROW(SQLException, "Invalid column name"); return (RETVAL); }


/* ------------------------------------------------------- Private methods */


static inline int getIndex(T R, const char *name) {
        int i;
        for (i = 0; i<R->columnCount; i++)
                if (Str_isByteEqual(name, R->columns[i].field->name))
                        return (i+1);
        return -1;
}


static inline void ensureCapacity(T R, int i) {
        if ((R->columns[i].real_length > R->bind[i].buffer_length)) {
                /* Column was truncated, resize and fetch column directly. */
                RESIZE(R->columns[i].buffer, R->columns[i].real_length + 1);
                R->bind[i].buffer = R->columns[i].buffer;
                R->bind[i].buffer_length = R->columns[i].real_length;
                /* Need to rebind as the buffer address has changed */
                if ((R->lastError = mysql_stmt_bind_result(R->stmt, R->bind)))
                        THROW(SQLException, "mysql_stmt_bind_result -- %s", mysql_stmt_error(R->stmt));
                /*
                 MySQL bug: mysql_stmt_fetch_column does not update bind buffer on blob/text data
                 See http://bugs.mysql.com/bug.php?id=33086 TODO find a workaround!
                 */
                if ((R->lastError = mysql_stmt_fetch_column(R->stmt, &R->bind[i], i, 0)))
                        THROW(SQLException, "mysql_stmt_fetch_column -- %s", mysql_stmt_error(R->stmt));
                // TODO remove when workaround is found
                if (strlen(R->columns[i].buffer) < R->columns[i].real_length)
                        DEBUG("MYSQL BUG: Buffer was truncated see http://bugs.mysql.com/bug.php?id=33086\n");
        }
}


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif


T MysqlResultSet_new(void *stmt, int maxRows, int keep) {
	T R;
	assert(stmt);
	NEW(R);
	R->stmt = stmt;
        R->keep = keep;
        R->maxRows = maxRows;
        R->columnCount = mysql_stmt_field_count(R->stmt);
        if ((R->columnCount <= 0) || ! (R->meta = mysql_stmt_result_metadata(R->stmt))) {
                DEBUG("Warning: column error - %s\n", mysql_stmt_error(stmt));
                R->stop = true;
        } else {
                int i;
                R->bind = CALLOC(R->columnCount, sizeof (MYSQL_BIND));
                R->columns = CALLOC(R->columnCount, sizeof (struct column_t));
                for (i = 0; i < R->columnCount; i++) {
                        R->columns[i].buffer = ALLOC(STRLEN + 1);
                        R->bind[i].buffer_type = MYSQL_TYPE_STRING;
                        R->bind[i].buffer = R->columns[i].buffer;
                        R->bind[i].buffer_length = STRLEN;
                        R->bind[i].is_null = &R->columns[i].is_null;
                        R->bind[i].length = &R->columns[i].real_length;
                        R->columns[i].field = mysql_fetch_field_direct(R->meta, i);
                }
                if ((R->lastError = mysql_stmt_bind_result(R->stmt, R->bind))) {
                        DEBUG("Warning: bind error - %s\n", mysql_stmt_error(stmt));
                        R->stop = true;
                }
        }
	return R;
}


void MysqlResultSet_free(T *R) {
        int i;
	assert(R && *R);
        for (i = 0; i < (*R)->columnCount; i++)
                FREE((*R)->columns[i].buffer);
        mysql_stmt_free_result((*R)->stmt);
        if ((*R)->keep==false)
                mysql_stmt_close((*R)->stmt);
        if ((*R)->meta)
                mysql_free_result((*R)->meta);
        FREE((*R)->columns);
        FREE((*R)->bind);
	FREE(*R);
}


int MysqlResultSet_getColumnCount(T R) {
	assert(R);
	return R->columnCount;
}


const char *MysqlResultSet_getColumnName(T R, int column) {
	int i;
	assert(R);
	i = column - 1;
	if (R->columnCount <= 0 ||
	   i < 0                ||
	   i > R->columnCount)
		return NULL;
	return R->columns[i].field->name;
}


int MysqlResultSet_next(T R) {
	assert(R);
        if (R->stop)
                return false;
        if (R->maxRows && (R->currentRow++ >= R->maxRows)) {
                R->stop = true;
#if MYSQL_VERSION_ID >= 50002
                /* Seems to need a cursor to work */
                mysql_stmt_reset(R->stmt); 
#else
                /* Bhaa! Where's my cancel method? 
                   Pencil neck mysql developers! */
                while (mysql_stmt_fetch(R->stmt)==0); 
#endif
                return false;
        }
        R->lastError = mysql_stmt_fetch(R->stmt);
        return ((R->lastError==MYSQL_OK) || (R->lastError==MYSQL_DATA_TRUNCATED));
}


long MysqlResultSet_getColumnSize(T R, int columnIndex) {
        TEST_INDEX(-1)
        return R->columns[i].real_length;
}


const char *MysqlResultSet_getString(T R, int columnIndex) {
        TEST_INDEX(NULL)
        ensureCapacity(R, i);
        R->columns[i].buffer[R->columns[i].real_length] = 0;
        return R->columns[i].buffer;
}


const char *MysqlResultSet_getStringByName(T R, const char *columnName) {
        GET_INDEX(NULL)
        return MysqlResultSet_getString(R, i);
}


int MysqlResultSet_getInt(T R, int columnIndex) {
        TEST_INDEX(0)
        return Str_parseInt(R->columns[i].buffer);
}


int MysqlResultSet_getIntByName(T R, const char *columnName) {
        GET_INDEX(0)
        return MysqlResultSet_getInt(R, i);
}


long long int MysqlResultSet_getLLong(T R, int columnIndex) {
        TEST_INDEX(0)
        return Str_parseLLong(R->columns[i].buffer);
}


long long int MysqlResultSet_getLLongByName(T R, const char *columnName) {
        GET_INDEX(0)
        return MysqlResultSet_getLLong(R, i);
}


double MysqlResultSet_getDouble(T R, int columnIndex) {
        TEST_INDEX(0.0)
        return Str_parseDouble(R->columns[i].buffer);
}


double MysqlResultSet_getDoubleByName(T R, const char *columnName) {
        GET_INDEX(0.0)
        return MysqlResultSet_getDouble(R, i);
}


const void *MysqlResultSet_getBlob(T R, int columnIndex, int *size) {
        TEST_INDEX(NULL)
        ensureCapacity(R, i);
        *size = R->columns[i].real_length;
        return R->columns[i].buffer;
}


const void *MysqlResultSet_getBlobByName(T R, const char *columnName, int *size) {
        GET_INDEX(NULL)
        return MysqlResultSet_getBlob(R, i, size);
}


int MysqlResultSet_readData(T R, int columnIndex, void *b, int l, long off) {
        int rc;
        TEST_INDEX(0)
        R->bind[i].buffer = b;
        R->bind[i].buffer_length = l;
        R->bind[i].length = &R->columns[i].real_length;
        if (off>R->columns[i].real_length)
                return 0;
        if ((rc = mysql_stmt_fetch_column(R->stmt, &R->bind[i], i, off))) {
                if (rc==CR_NO_DATA)
                        return 0;
                THROW(SQLException, "mysql_stmt_fetch_column -- %s", mysql_stmt_error(R->stmt));
                return -1;
        }
        return (R->columns[i].real_length-off)>l?l:(R->columns[i].real_length-off);
}


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif

