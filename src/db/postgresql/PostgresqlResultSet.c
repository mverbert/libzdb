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
#include <string.h>
#include <sys/types.h>
#include <libpq-fe.h>

#include "ResultSetStrategy.h"
#include "PostgresqlResultSet.h"


/**
 * Implementation of the ResultSet/Strategy interface for postgresql. 
 * Accessing columns with index outside range throws SQLException
 *
 * @file
 */


/* ----------------------------------------------------------- Definitions */


const struct Rop_T postgresqlrops = {
        "postgresql",
        PostgresqlResultSet_free,
        PostgresqlResultSet_getColumnCount,
        PostgresqlResultSet_getColumnName,
        PostgresqlResultSet_next,
        PostgresqlResultSet_getColumnSize,
        PostgresqlResultSet_getString,
        PostgresqlResultSet_getBlob,
        PostgresqlResultSet_readData
};

#define T ResultSetImpl_T
struct T {
        int keep;
        int maxRows;
        int currentRow;
        int columnCount;
        int rowCount;
        PGresult *res;
};

#define TEST_INDEX(RETVAL) \
        int i; assert(R); i= columnIndex - 1; if (R->columnCount <= 0 || \
        i < 0 || i >= R->columnCount) { THROW(SQLException, "Column index is out of range"); return(RETVAL); } \
        if (PQgetisnull(R->res, R->currentRow, i)) return (RETVAL); 


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T PostgresqlResultSet_new(void *res, int maxRows, int keep) {
        T R;
        assert(res);
        NEW(R);
        R->res = res;
        R->keep = keep;
        R->maxRows = maxRows;
        R->currentRow = -1;
        R->columnCount = PQnfields(R->res);
        R->rowCount = PQntuples(R->res);
        return R;
}


void PostgresqlResultSet_free(T *R) {
        assert(R && *R);
        FREE(*R);
}


int PostgresqlResultSet_getColumnCount(T R) {
        assert(R);
        return R->columnCount;
}


const char *PostgresqlResultSet_getColumnName(T R, int column) {
        assert(R);
        column--;
        if (R->columnCount <= 0 ||
            column < 0          ||
            column > R->columnCount)
                return NULL;
        return PQfname(R->res, column);
}


int PostgresqlResultSet_next(T R) {
        assert(R);
        return (! ((R->currentRow++ >= (R->rowCount - 1)) || (R->maxRows && (R->currentRow >= R->maxRows))));
}


long PostgresqlResultSet_getColumnSize(T R, int columnIndex) {
        TEST_INDEX(-1)
        return PQgetlength(R->res, R->currentRow, i);
}


const char *PostgresqlResultSet_getString(T R, int columnIndex) {
        TEST_INDEX(NULL)
        return PQgetvalue(R->res, R->currentRow, i);
}


const void *PostgresqlResultSet_getBlob(T R, int columnIndex, int *size) {
        TEST_INDEX(NULL)
        *size = PQgetlength(R->res, R->currentRow, i);
        return PQgetvalue(R->res, R->currentRow, i);
}


int PostgresqlResultSet_readData(T R, int columnIndex, void *b, int l, long off) {
        long r;
        int size;
        const void *blob;
        TEST_INDEX(0)
        blob = PQgetvalue(R->res, R->currentRow, i);
        size = PQgetlength(R->res, R->currentRow, i);
        if (off > size)
                return 0;
        r = off + l > size ? size - off : l;
        memcpy(b, blob + off, r);
        return r;
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif

