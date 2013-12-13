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
#include "system/Time.h"


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


int ResultSet_isnull(T R, int columnIndex) {
        assert(R);
        return R->op->isnull(R->D, columnIndex);
}


#pragma mark - Columns


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


long ResultSet_getLong(T R, int columnIndex) {
	assert(R);
        const char *s = R->op->getString(R->D, columnIndex);
	return s ? Str_parseLong(s) : 0;
}


long ResultSet_getLongByName(T R, const char *columnName) {
	assert(R);
	return ResultSet_getLong(R, getIndex(R, columnName));
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


#pragma mark - Date and Time


time_t ResultSet_getTimestamp(T R, int columnIndex) {
        return (time_t)ResultSet_getLong(R, columnIndex);
}


time_t ResultSet_getTimestampByName(T R, const char *columnName) {
        return (time_t)ResultSet_getLong(R, getIndex(R, columnName));
}


sqldate_t ResultSet_getDate(T R, int columnIndex) {
        assert(R);
        sqldate_t r = {.year = 0};
        const char *t = ResultSet_getString(R, columnIndex);
        if (STR_DEF(t))
                Time_toDate(t, &r);
        return r;
}


sqldate_t ResultSet_getDateByName(T R, const char *columnName) {
        assert(R);
        return ResultSet_getDate(R, getIndex(R, columnName));
}


sqltime_t ResultSet_getTime(T R, int columnIndex) {
        assert(R);
        sqltime_t r = {.hour = 0};
        const char *t = ResultSet_getString(R, columnIndex);
        if (STR_DEF(t))
                Time_toTime(t, &r);
        return r;
}


sqltime_t ResultSet_getTimeByName(T R, const char *columnName) {
        assert(R);
        return ResultSet_getTime(R, getIndex(R, columnName));
}


sqldatetime_t ResultSet_getDateTime(T R, int columnIndex) {
        assert(R);
        sqldatetime_t r = {.time.hour = 0};
        const char *t = ResultSet_getString(R, columnIndex);
        if (STR_DEF(t))
                Time_toDateTime(t, &r);
        return r;
}


sqldatetime_t ResultSet_getDateTimeByName(T R, const char *columnName) {
        assert(R);
        return ResultSet_getDateTime(R, getIndex(R, columnName));
}

