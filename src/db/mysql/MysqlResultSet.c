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
#include <string.h>
#include <mysql.h>
#include <errmsg.h>

#include "ResultSetDelegate.h"
#include "MysqlResultSet.h"


/**
 * Implementation of the ResultSet/Delegate interface for mysql. 
 * Accessing columns with index outside range throws SQLException
 *
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#define MYSQL_OK 0

const struct Rop_T mysqlrops = {
	"mysql",
        MysqlResultSet_free,
        MysqlResultSet_getColumnCount,
        MysqlResultSet_getColumnName,
        MysqlResultSet_next,
        MysqlResultSet_getColumnSize,
        MysqlResultSet_getString,
        MysqlResultSet_getBlob,
};

typedef struct column_t {
        my_bool is_null;
        MYSQL_FIELD *field;
        unsigned long real_length;
        char *buffer;
} *column_t;

#define T ResultSetDelegate_T
struct T {
        int stop;
        int keep;
        int maxRows;
        int lastError;
        int needRebind;
	int currentRow;
	int columnCount;
        MYSQL_RES *meta;
        MYSQL_BIND *bind;
	MYSQL_STMT *stmt;
        column_t columns;
};

#define TEST_INDEX \
        int i; assert(R);i = columnIndex-1; if (R->columnCount <= 0 || \
        i < 0 || i >= R->columnCount) { THROW(SQLException, "Column index is out of range"); }  


/* ------------------------------------------------------- Private methods */


static inline void ensureCapacity(T R, int i) {
        if ((R->columns[i].real_length > R->bind[i].buffer_length)) {
                /* Column was truncated, resize and fetch column directly. */
                RESIZE(R->columns[i].buffer, R->columns[i].real_length + 1);
                R->bind[i].buffer = R->columns[i].buffer;
                R->bind[i].buffer_length = R->columns[i].real_length;
                if ((R->lastError = mysql_stmt_fetch_column(R->stmt, &R->bind[i], i, 0)))
                        THROW(SQLException, "mysql_stmt_fetch_column -- %s", mysql_stmt_error(R->stmt));
                R->needRebind = true;
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
                R->bind = CALLOC(R->columnCount, sizeof (MYSQL_BIND));
                R->columns = CALLOC(R->columnCount, sizeof (struct column_t));
                for (int i = 0; i < R->columnCount; i++) {
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
	assert(R && *R);
        for (int i = 0; i < (*R)->columnCount; i++)
                FREE((*R)->columns[i].buffer);
        mysql_stmt_free_result((*R)->stmt);
        if ((*R)->keep == false)
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
	assert(R);
	column--;
	if (R->columnCount <= 0 ||
	   column < 0           ||
	   column > R->columnCount)
		return NULL;
	return R->columns[column].field->name;
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
                while (mysql_stmt_fetch(R->stmt) == 0); 
#endif
                return false;
        }
        if (R->needRebind) {
                if ((R->lastError = mysql_stmt_bind_result(R->stmt, R->bind)))
                        THROW(SQLException, "mysql_stmt_bind_result -- %s", mysql_stmt_error(R->stmt));
                R->needRebind = false;
        }
        R->lastError = mysql_stmt_fetch(R->stmt);
        if (R->lastError == 1)
                THROW(SQLException, "mysql_stmt_fetch -- %s", mysql_stmt_error(R->stmt));
        return ((R->lastError == MYSQL_OK) || (R->lastError == MYSQL_DATA_TRUNCATED));
}


long MysqlResultSet_getColumnSize(T R, int columnIndex) {
        TEST_INDEX
        if (R->columns[i].is_null) 
                return 0;
        return R->columns[i].real_length;
}


const char *MysqlResultSet_getString(T R, int columnIndex) {
        TEST_INDEX
        if (R->columns[i].is_null) 
                return NULL;
        ensureCapacity(R, i);
        R->columns[i].buffer[R->columns[i].real_length] = 0;
        return R->columns[i].buffer;
}


const void *MysqlResultSet_getBlob(T R, int columnIndex, int *size) {
        TEST_INDEX
        if (R->columns[i].is_null) 
                return NULL;
        ensureCapacity(R, i);
        *size = (int)R->columns[i].real_length;
        return R->columns[i].buffer;
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif

