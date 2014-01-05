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


#ifndef RESULTSETDELEGATE_INCLUDED
#define RESULTSETDELEGATE_INCLUDED
#include "system/Time.h"

/**
 * This interface defines the <b>contract</b> for the concrete database 
 * implementation used for delegation in the ResultSet class.
 *
 * @file
 */ 

#define T ResultSetDelegate_T
typedef struct T *T;

typedef struct Rop_T {
        const char *name;
        void (*free)(T *R);
        int (*getColumnCount)(T R);
        const char *(*getColumnName)(T R, int columnIndex);
        long (*getColumnSize)(T R, int columnIndex);
        int (*next)(T R);
        int (*isnull)(T R, int columnIndex);
        const char *(*getString)(T R, int columnIndex);
        const void *(*getBlob)(T R, int columnIndex, int *size);
        time_t (*getTimestamp)(T R, int columnIndex);
        struct tm *(*getDateTime)(T R, int columnIndex, struct tm *tm);
} *Rop_T;

/**
 * Throws exception if columnIndex is outside the columnCount range.
 * @return columnIndex - 1. In the API, columnIndex starts with 1,
 * internally it starts with 0.
 */
static inline int checkAndSetColumnIndex(int columnIndex, int columnCount) {
        int i = columnIndex - 1;
        if (columnCount <= 0 || i < 0 || i >= columnCount)
                THROW(SQLException, "Column index is out of range");
        return i;
}

#undef T
#endif
