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

#include "system/Time.h"
#include "ResultSet.h"
#include "PreparedStatement.h"


/**
 * Implementation of the PreparedStatement interface 
 *
 * @file
 */


/* ----------------------------------------------------------- Definitions */


typedef struct param_t {
        char timestamp[20];
} *param_t;
#define T PreparedStatement_T
struct PreparedStatement_S {
        Pop_T op;
        int paramCount;
        ResultSet_T resultSet;
        PreparedStatementDelegate_T D;
        param_t params;
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

T PreparedStatement_new(PreparedStatementDelegate_T D, Pop_T op, int paramCount) {
	T P;
	assert(D);
	assert(op);
        P = CALLOC(1, (sizeof(*P) + paramCount * sizeof(P->params[0])));
	P->D = D;
	P->op = op;
        P->paramCount = paramCount;
        P->params = (param_t)(P + 1);
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
#pragma mark - Parameters


void PreparedStatement_setString(T P, int parameterIndex, const char *x) {
	assert(P);
        P->op->setString(P->D, parameterIndex, x);
}


void PreparedStatement_setInt(T P, int parameterIndex, int x) {
	assert(P);
        P->op->setInt(P->D, parameterIndex, x);
}


void PreparedStatement_setLong(T P, int parameterIndex, long x) {
	assert(P);
        P->op->setLong(P->D, parameterIndex, x);
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


void PreparedStatement_setTimestamp(T P, int parameterIndex, time_t x) {
        assert(P);
        int i = checkAndSetParameterIndex(parameterIndex, P->paramCount);
        P->op->setString(P->D, parameterIndex, Time_toString(x, P->params[i].timestamp));
}


#pragma mark - Methods


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


long long int PreparedStatement_rowsChanged(T P) {
        assert(P);
        return P->op->rowsChanged(P->D);
}


#pragma mark - Properties


int PreparedStatement_getParameterCount(T P) {
        assert(P);
        return P->paramCount;
}
