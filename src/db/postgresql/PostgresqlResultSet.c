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
#include <ctype.h>
#include <sys/types.h>

#include "PostgresqlAdapter.h"


/**
 * Implementation of the ResultSet/Delegate interface for postgresql.
 * Accessing columns with index outside range throws SQLException
 *
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#define T ResultSetDelegate_T
struct T {
        int maxRows;
        int rowCount;
        int currentRow;
        int columnCount;
        PGresult *res;
        Connection_T delegator;
};
#define ISFIRSTOCTDIGIT(CH) ((CH) >= '0' && (CH) <= '3')
#define ISOCTDIGIT(CH) ((CH) >= '0' && (CH) <= '7')
#define OCTVAL(CH) ((CH) - '0')


/* ------------------------------------------------------- Private methods */


/* Unescape the buffer pointed to by s 'in-place' using the (un)escape mechanizm
 described at http://www.postgresql.org/docs/9.0/static/datatype-binary.html
 The new size of s is assigned to r. Returns s. See _getBlob()
 below for usage and further info. See also Postgres' PQunescapeBytea() function
 which this function mirrors except it does not allocate a new string.
 */
static inline const void *_unescape_bytea(uchar_t *s, int len, int *r) {
        assert(s);
        register int i, j;
        if (s[0] == '\\' && s[1] == 'x') { // bytea hex format
                static const uchar_t hex[128] = {
                        0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0,  1,  2,  3,  4,  5,  6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
                        0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                };
                for (i = 0, j = 2; j < len; j++) {
                        // Whitespace between hex pairs is allowed 🤔
                        if (isxdigit(s[j])) {
                                s[i] = hex[s[j]] << 4;
                                s[i] |= hex[s[j + 1]];
                                i++;
                                j++;
                        }
                }
        } else { // bytea escaped format
                uchar_t byte;
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
        }
        *r = i;
        if (i < j)
                s[i] = 0; // If unescape was performed, terminate the buffer to mirror postgres behavior
        return s;
}

/* ------------------------------------------------------------- Constructor */


T PostgresqlResultSet_new(Connection_T delegator, PGresult *res) {
        T R;
        assert(delegator);
        NEW(R);
        R->delegator = delegator;
        R->res = res;
        R->maxRows = Connection_getMaxRows(delegator);
        R->currentRow = -1;
        R->columnCount = PQnfields(R->res);
        R->rowCount = PQntuples(R->res);
        return R;
}


/* -------------------------------------------------------- Delegate methods */


static void _free(T *R) {
        assert(R && *R);
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
        return PQfname(R->res, columnIndex);
}


static long _getColumnSize(T R, int columnIndex) {
        int i = checkAndSetColumnIndex(columnIndex, R->columnCount);
        if (PQgetisnull(R->res, R->currentRow, i))
                return 0;
        return PQgetlength(R->res, R->currentRow, i);
}


static bool _next(T R) {
        assert(R);
        R->currentRow += 1;
        return (! ((R->currentRow >= R->rowCount) || (R->maxRows && (R->currentRow >= R->maxRows))));
}


static bool _isnull(T R, int columnIndex) {
        assert(R);
        int i = checkAndSetColumnIndex(columnIndex, R->columnCount);
        return PQgetisnull(R->res, R->currentRow, i);
}


static const char *_getString(T R, int columnIndex) {
        assert(R);
        int i = checkAndSetColumnIndex(columnIndex, R->columnCount);
        if (PQgetisnull(R->res, R->currentRow, i))
                return NULL;
        return PQgetvalue(R->res, R->currentRow, i);
}


/*
 * As a "hack" to avoid extra allocation and complications by using PQunescapeBytea()
 * we instead unescape the buffer retrieved via PQgetvalue 'in-place'. This should
 * be safe as unescape will only modify internal bytes in the buffer and not change
 * the buffer pointer. See also unescape_bytea() above.
 */
static const void *_getBlob(T R, int columnIndex, int *size) {
        assert(R);
        int i = checkAndSetColumnIndex(columnIndex, R->columnCount);
        if (PQgetisnull(R->res, R->currentRow, i))
                return NULL;
        return _unescape_bytea((uchar_t*)PQgetvalue(R->res, R->currentRow, i), PQgetlength(R->res, R->currentRow, i), size);
}


/* ------------------------------------------------------------------------- */


const struct Rop_T postgresqlrops = {
        .name           = "postgresql",
        .free           = _free,
        .getColumnCount = _getColumnCount,
        .getColumnName  = _getColumnName,
        .getColumnSize  = _getColumnSize,
        .next           = _next,
        .isnull         = _isnull,
        .getString      = _getString,
        .getBlob        = _getBlob
        // get/setFetchSize is not applicable for Postgres or rather libpq
        // getTimestamp and getDateTime is handled in ResultSet
};

