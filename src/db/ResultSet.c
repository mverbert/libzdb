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

#include "ResultSet.h"


/**
 * Implementation of the ResultSet interface 
 *
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#define T ResultSet_T
struct ResultSet_S {
        Rop_T op;
        ResultSetDelegate_T D;
};


/* ------------------------------------------------------- Private methods */


static inline int getIndex(T R, const char *name) {
        int i;
        int columns = ResultSet_getColumnCount(R);
        for (i = 1; i <= columns; i++)
                if (Str_isByteEqual(name, ResultSet_getColumnName(R, i)))
                        return i;
        THROW(SQLException, "Invalid column name '%s'", name ? name : "null");
        return -1;
}


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T ResultSet_new(ResultSetDelegate_T D, Rop_T op) {
	T R;
	assert(D);
	assert(op);
	NEW(R);
	R->D = D;
	R->op = op;
	return R;
}


void ResultSet_free(T *R) {
	assert(R && *R);
        (*R)->op->free(&(*R)->D);
	FREE(*R);
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif


/* ------------------------------------------------------------ Properties */


int ResultSet_getColumnCount(T R) {
	assert(R);
	return R->op->getColumnCount(R->D);
}


const char *ResultSet_getColumnName(T R, int columnIndex) {
	assert(R);
	return R->op->getColumnName(R->D, columnIndex);
}


long ResultSet_getColumnSize(T R, int columnIndex) {
	assert(R);
	return R->op->getColumnSize(R->D, columnIndex);
}


/* -------------------------------------------------------- Public methods */


int ResultSet_next(T R) {
        return R ? R->op->next(R->D) : false;
}


const char *ResultSet_getString(T R, int columnIndex) {
	assert(R);
	return R->op->getString(R->D, columnIndex);
}


const char *ResultSet_getStringByName(T R, const char *columnName) {
	assert(R);
	return ResultSet_getString(R, getIndex(R, columnName));
}


int ResultSet_getInt(T R, int columnIndex) {
	assert(R);
        const char *s = R->op->getString(R->D, columnIndex);
	return s ? Str_parseInt(s) : 0;
}


int ResultSet_getIntByName(T R, const char *columnName) {
	assert(R);
	return ResultSet_getInt(R, getIndex(R, columnName));
}


long long int ResultSet_getLLong(T R, int columnIndex) {
	assert(R);
        const char *s = R->op->getString(R->D, columnIndex);
	return s ? Str_parseLLong(s) : 0;
}


long long int ResultSet_getLLongByName(T R, const char *columnName) {
	assert(R);
	return ResultSet_getLLong(R, getIndex(R, columnName));
}


double ResultSet_getDouble(T R, int columnIndex) {
	assert(R);
        const char *s = R->op->getString(R->D, columnIndex);
	return s ? Str_parseDouble(s) : 0.0;
}


double ResultSet_getDoubleByName(T R, const char *columnName) {
	assert(R);
	return ResultSet_getDouble(R, getIndex(R, columnName));
}


const void *ResultSet_getBlob(T R, int columnIndex, int *size) {
	assert(R);
        const void *b = R->op->getBlob(R->D, columnIndex, size);
        if (! b)
                *size = 0;
	return b;
}


const void *ResultSet_getBlobByName(T R, const char *columnName, int *size) {
	assert(R);
	return ResultSet_getBlob(R, getIndex(R, columnName), size);
}

