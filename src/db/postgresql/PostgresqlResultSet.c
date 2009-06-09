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
};

#define T ResultSetImpl_T
struct T {
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

#define ISFIRSTOCTDIGIT(CH) ((CH) >= '0' && (CH) <= '3')
#define ISOCTDIGIT(CH) ((CH) >= '0' && (CH) <= '7')
#define OCTVAL(CH) ((CH) - '0')


/* ------------------------------------------------------- Private methods */


/* Unescape the buffer pointed to by s 'ìn-place' using the (un)escape mechanizm
 described at http://www.postgresql.org/docs/8.3/interactive/datatype-binary.html
 The new size of s is assigned to r. Returns s. See PostgresqlResultSet_getBlob()
 below for usage and further info.
 */
static inline const void *unescape_bytea(uchar_t *s, int len, int *r) {
        int byte;
        register int i, j;
        assert(s);
        for (i = j = 0; j < len; i++, j++) {
                if ((s[i] = s[j]) == '\\') {
                        if (s[j + 1] == '\\')
                                j++;
                        else if ((ISFIRSTOCTDIGIT(s[j + 1])) 
                                   && (ISOCTDIGIT(s[j + 2])) 
                                   && (ISOCTDIGIT(s[j + 3]))) {
                                byte = OCTVAL(s[j + 1]);
                                byte = (byte << 3) + OCTVAL(s[j + 2]);
                                byte = (byte << 3) + OCTVAL(s[j + 3]);
                                s[i] = byte;
                                j += 3;
                        }
                }
        }
        *r = i;
        s[i] = 0; // Terminate the buffer to mirror postgres behavior
        return s;
}


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T PostgresqlResultSet_new(void *res, int maxRows) {
        T R;
        assert(res);
        NEW(R);
        R->res = res;
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


/*
 * Nota bene: In libzdb we have standardized throughout to retrieve results
 * as text/byte, not as binary to avoid platform conversion problems and
 * to be general. Although for some columns, such as a blob, we would like 
 * to retrieve the value as binary, unfortunately Postgres does not provide
 * means to retrieve a certain column as binary while others are text. In 
 * Postgres all columns in a result set are either retrieved as binary or 
 * as text and the result format must be specified at SQL command execution 
 * time. This means that Postgres will escape a bytea column since we retrieve 
 * result as text and we must unescape the value again to get the actual binary 
 * value. This escape/unescape operation is unfortunate but necessary as long 
 * as postgres insist on escaping blobs and does not provide means to get a
 * binary value directly via an API call. See also unescape_bytea() above.
 * 
 * As a hack to avoid extra allocation we unescape the buffer retrieved via
 * PQgetvalue 'in-place'. This should be safe as unescape will only modify 
 * internal bytes in the buffer and not change the buffer pointer nor expand
 * the buffer. That is, as long as Postgres does not change the escaping mechanizm. 
 */
const void *PostgresqlResultSet_getBlob(T R, int columnIndex, int *size) {
        TEST_INDEX(NULL)
        return unescape_bytea((uchar_t*)PQgetvalue(R->res, R->currentRow, i), PQgetlength(R->res, R->currentRow, i), size);
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif

