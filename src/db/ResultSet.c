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

#include "URL.h"
#include "ResultSet.h"


/**
 * Implementation of the ResultSet interface 
 *
 * @version \$Id: ResultSet.c,v 1.30 2008/03/20 11:28:53 hauk Exp $
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#define T ResultSet_T
struct T {
        Rop_T op;
        IResultSet_T I;
};


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T ResultSet_new(IResultSet_T I, Rop_T op) {
	T R;
	assert(I);
	assert(op);
	NEW(R);
	R->I = I;
	R->op = op;
	return R;
}


void ResultSet_free(T *R) {
	assert(R && *R);
        (*R)->op->free(&(*R)->I);
	FREE(*R);
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif


/* ------------------------------------------------------------ Properties */


int ResultSet_getColumnCount(T R) {
	assert(R);
	return R->op->getColumnCount(R->I);
}


const char *ResultSet_getColumnName(T R, int column) {
	assert(R);
	return R->op->getColumnName(R->I, column);
}


long ResultSet_getColumnSize(T R, int columnIndex) {
	assert(R);
	return R->op->getColumnSize(R->I, columnIndex);
}


/* -------------------------------------------------------- Public methods */


int ResultSet_next(T R) {
        return R ? R->op->next(R->I) : false;
}


const char *ResultSet_getString(T R, int columnIndex) {
	assert(R);
	return R->op->getString(R->I, columnIndex);
}


const char *ResultSet_getStringByName(T R, const char *columnName) {
	assert(R);
	return R->op->getStringByName(R->I, columnName);
}


int ResultSet_getInt(T R, int columnIndex) {
	assert(R);
	return R->op->getInt(R->I, columnIndex);
}


int ResultSet_getIntByName(T R, const char *columnName) {
	assert(R);
	return R->op->getIntByName(R->I, columnName);
}


long long int ResultSet_getLLong(T R, int columnIndex) {
	assert(R);
	return R->op->getLLong(R->I, columnIndex);
}


long long int ResultSet_getLLongByName(T R, const char *columnName) {
	assert(R);
	return R->op->getLLongByName(R->I, columnName);
}


double ResultSet_getDouble(T R, int columnIndex) {
	assert(R);
	return R->op->getDouble(R->I, columnIndex);
}


double ResultSet_getDoubleByName(T R, const char *columnName) {
	assert(R);
	return R->op->getDoubleByName(R->I, columnName);
}


const void *ResultSet_getBlob(T R, int columnIndex, int *size) {
	assert(R);
	return R->op->getBlob(R->I, columnIndex, size);
}


const void *ResultSet_getBlobByName(T R, const char *columnName, int *size) {
	assert(R);
	return R->op->getBlobByName(R->I, columnName, size);
}


int ResultSet_readData(T R, int columnIndex, void *b, int length, long off) {
        assert(R);
        assert(b);
	return R->op->readData(R->I, columnIndex, b, length, off);
}
