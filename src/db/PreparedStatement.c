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

#include "Vector.h"
#include "ResultSet.h"
#include "PreparedStatement.h"


/**
 * Implementation of the PreparedStatement interface 
 *
 * @version \$Id: PreparedStatement.c,v 1.19 2008/03/20 11:28:53 hauk Exp $
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#define T PreparedStatement_T
struct T {
        Pop_T op;
        ResultSet_T resultSet;
        IPreparedStatement_T I;
};


/* ------------------------------------------------------- Private methods */


static void clearResultSet(T P) {
        if (P->resultSet)
                ResultSet_free(&P->resultSet);
}


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T PreparedStatement_new(IPreparedStatement_T I, Pop_T op) {
	T P;
	assert(I);
	assert(op);
	NEW(P);
	P->I = I;
	P->op = op;
	return P;
}


void PreparedStatement_free(T *P) {
	assert(P && *P);
        clearResultSet((*P));
        (*P)->op->free(&(*P)->I);
	FREE(*P);
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif


/* -------------------------------------------------------- Public methods */


int PreparedStatement_setString(T P, int parameterIndex, const char *x) {
	assert(P);
        if (P->op->setString(P->I, parameterIndex, x))
                return true;
        THROW(SQLException, "PreparedStatement_setString");
        return false;
}


int PreparedStatement_setInt(T P, int parameterIndex, int x) {
	assert(P);
	if (P->op->setInt(P->I, parameterIndex, x))
                return true;
        THROW(SQLException, "PreparedStatement_setInt");
        return false;
}


int PreparedStatement_setLLong(T P, int parameterIndex, long long int x) {
	assert(P);
	if (P->op->setLLong(P->I, parameterIndex, x))
                return true;
        THROW(SQLException, "PreparedStatement_setLLong");
        return false;
}


int PreparedStatement_setDouble(T P, int parameterIndex, double x) {
	assert(P);
	if (P->op->setDouble(P->I, parameterIndex, x))
                return true;
        THROW(SQLException, "PreparedStatement_setDouble");
        return false;
}


int PreparedStatement_setBlob(T P, int parameterIndex, const void *x, int size) {
	assert(P);
	if (P->op->setBlob(P->I, parameterIndex, x, size))
                return true;
        THROW(SQLException, "PreparedStatement_setBlob");
        return false;
}


int PreparedStatement_execute(T P) {
	assert(P);
        clearResultSet(P);
	if (P->op->execute(P->I))
                return true;
        THROW(SQLException, "PreparedStatement_execute");
        return false;
}


ResultSet_T PreparedStatement_executeQuery(T P) {
	assert(P);
        clearResultSet(P);
	P->resultSet = P->op->executeQuery(P->I);
        if (! P->resultSet)
                THROW(SQLException, "PreparedStatement_executeQuery");
        return P->resultSet;
}

