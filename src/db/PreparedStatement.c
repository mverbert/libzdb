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
 */


#include "Config.h"

#include <stdio.h>

#include "ResultSet.h"
#include "PreparedStatement.h"


/**
 * Implementation of the PreparedStatement interface 
 *
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#define T PreparedStatement_T
struct PreparedStatement_S {
        Pop_T op;
        ResultSet_T resultSet;
        PreparedStatementDelegate_T D;
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

T PreparedStatement_new(PreparedStatementDelegate_T D, Pop_T op) {
	T P;
	assert(D);
	assert(op);
	NEW(P);
	P->D = D;
	P->op = op;
	return P;
}


void PreparedStatement_free(T *P) {
	assert(P && *P);
        clearResultSet((*P));
        (*P)->op->free(&(*P)->D);
	FREE(*P);
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif


/* -------------------------------------------------------- Public methods */


void PreparedStatement_setString(T P, int parameterIndex, const char *x) {
	assert(P);
        P->op->setString(P->D, parameterIndex, x);
}


void PreparedStatement_setInt(T P, int parameterIndex, int x) {
	assert(P);
        P->op->setInt(P->D, parameterIndex, x);
}


void PreparedStatement_setLLong(T P, int parameterIndex, long long int x) {
	assert(P);
        P->op->setLLong(P->D, parameterIndex, x);
}


void PreparedStatement_setDouble(T P, int parameterIndex, double x) {
	assert(P);
        P->op->setDouble(P->D, parameterIndex, x);
}


void PreparedStatement_setBlob(T P, int parameterIndex, const void *x, int size) {
	assert(P);
        P->op->setBlob(P->D, parameterIndex, x, size);
}


void PreparedStatement_execute(T P) {
	assert(P);
        clearResultSet(P);
        P->op->execute(P->D);
}


ResultSet_T PreparedStatement_executeQuery(T P) {
	assert(P);
        clearResultSet(P);
	P->resultSet = P->op->executeQuery(P->D);
        if (! P->resultSet)
                THROW(SQLException, "PreparedStatement_executeQuery");
        return P->resultSet;
}

