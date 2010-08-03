/*
 * Copyright (C) 2004-2010 Tildeslash Ltd. All rights reserved.
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

#include "Vector.h"
#include "ResultSet.h"
#include "PreparedStatement.h"


/**
 * Implementation of the PreparedStatement interface 
 *
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#define T PreparedStatement_T
struct T {
        Pop_T op;
        ResultSet_T resultSet;
        PreparedStatementImpl_T I;
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

T PreparedStatement_new(PreparedStatementImpl_T I, Pop_T op) {
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


void PreparedStatement_setString(T P, int parameterIndex, const char *x) {
	assert(P);
        P->op->setString(P->I, parameterIndex, x);
}


void PreparedStatement_setInt(T P, int parameterIndex, int x) {
	assert(P);
        P->op->setInt(P->I, parameterIndex, x);
}


void PreparedStatement_setLLong(T P, int parameterIndex, long long int x) {
	assert(P);
        P->op->setLLong(P->I, parameterIndex, x);
}


void PreparedStatement_setDouble(T P, int parameterIndex, double x) {
	assert(P);
        P->op->setDouble(P->I, parameterIndex, x);
}


void PreparedStatement_setBlob(T P, int parameterIndex, const void *x, int size) {
	assert(P);
        P->op->setBlob(P->I, parameterIndex, x, size);
}


int PreparedStatement_execute(T P) {
	assert(P);
        clearResultSet(P);
        P->op->execute(P->I);
	return true;
}


ResultSet_T PreparedStatement_executeQuery(T P) {
	assert(P);
        clearResultSet(P);
	P->resultSet = P->op->executeQuery(P->I);
        if (! P->resultSet)
                THROW(SQLException, "PreparedStatement_executeQuery");
        return P->resultSet;
}

