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


#ifndef RESULTSETSTRATEGY_INCLUDED
#define RESULTSETSTRATEGY_INCLUDED


/**
 * This interface defines the <b>contract</b> for the concrete database 
 * implementation used for delegation in the ResultSet class.
 *
 * @file
 */ 

#define T ResultSetImpl_T
typedef struct T *T;

typedef struct Rop_T {
        char *name;
        void (*free)(T *R);
        int (*getColumnCount)(T R);
        const char *(*getColumnName)(T R, int column);
        int (*next)(T R);
        long (*getColumnSize)(T R, int columnIndex);
        const char *(*getString)(T R, int columnIndex);
        const char *(*getStringByName)(T R, const char *columnName);
        int (*getInt)(T R, int columnIndex);
        int (*getIntByName)(T R, const char *columnName);
        long long int (*getLLong)(T R, int columnIndex);
        long long int (*getLLongByName)(T R, const char *columnName);
        double (*getDouble)(T R, int columnIndex);
        double (*getDoubleByName)(T R, const char *columnName);
        const void *(*getBlob)(T R, int columnIndex, int *size);
        const void *(*getBlobByName)(T R, const char *columnName, int *size);
        int (*readData)(T R, int columnIndex, void *b, int length, long off);
} *Rop_T;

#undef T
#endif
