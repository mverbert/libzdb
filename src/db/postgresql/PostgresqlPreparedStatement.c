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


#include "Config.h"

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <libpq-fe.h>

#include "ResultSet.h"
#include "PostgresqlResultSet.h"
#include "PreparedStatementStrategy.h"
#include "PostgresqlPreparedStatement.h"


/**
 * Implementation of the PreparedStatement/Strategy interface for postgresql.
 *
 * @version \$Id: PostgresqlPreparedStatement.c,v 1.11 2008/03/20 11:28:53 hauk Exp $
 * @file
 */


/* ----------------------------------------------------------- Definitions */


const struct Pop_T postgresqlpops = {
        "postgresql",
        PostgresqlPreparedStatement_free,
        PostgresqlPreparedStatement_setString,
        PostgresqlPreparedStatement_setInt,
        PostgresqlPreparedStatement_setLLong,
        PostgresqlPreparedStatement_setDouble,
        PostgresqlPreparedStatement_setBlob,
        PostgresqlPreparedStatement_execute,
        PostgresqlPreparedStatement_executeQuery
};

#define T PreparedStatementImpl_T
struct T {
        int maxRows;
        int lastError;
        char *stmt;
        PGconn *db;
        PGresult *res;
        int paramCount;
        char **paramValues; 
        int *paramLengths; 
        int *paramFormats;
};

#ifndef net_buffer_length
#define net_buffer_length 4096
#endif
#define TEST_INDEX \
        int i; assert(P); i= parameterIndex - 1; if (P->paramCount <= 0 || \
        i < 0 || i >= P->paramCount) THROW(SQLException, "Parameter index out of range");

extern const struct Rop_T postgresqlrops;


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T PostgresqlPreparedStatement_new(PGconn *db, int maxRows, char *stmt, int prm) {
        T P;
        assert(stmt);
        NEW(P);
        P->maxRows = maxRows;
        P->lastError = PGRES_COMMAND_OK;
        P->stmt = stmt;
        P->db = db;
        P->res = NULL;
        P->paramCount = prm;
        if (P->paramCount) {
                P->paramValues = CALLOC(prm, sizeof(char *));
                P->paramLengths = CALLOC(prm, sizeof(int));
                P->paramFormats = CALLOC(prm, sizeof(int));
        }
        return P;
}


void PostgresqlPreparedStatement_free(T *P) {
        char stmt[STRLEN];
	assert(P && *P);
        /* NOTE: there is no C API function for explicit statement
         * deallocation (postgres-8.1.x) - the DEALLOCATE statement
         * has to be used. The postgres documentation mentiones such
         * function as a possible future extension */
        snprintf(stmt, STRLEN, "DEALLOCATE \"%s\";", (*P)->stmt);
        PQclear(PQexec((*P)->db, stmt));
        PQclear((*P)->res);
	FREE((*P)->stmt);
        if ((*P)->paramCount) {
                int i;
                for (i = 0; i < (*P)->paramCount; i++) {
                        if ((*P)->paramFormats[i] == 0) {
	                        FREE((*P)->paramValues[i]);
                        }
                }
	        FREE((*P)->paramValues);
	        FREE((*P)->paramLengths);
	        FREE((*P)->paramFormats);
        }
	FREE(*P);
}


void PostgresqlPreparedStatement_setString(T P, int parameterIndex, const char *x) {
        TEST_INDEX
        if (x==NULL) {
                P->paramValues[i] = NULL;
                P->paramLengths[i] = 0;
        } else {
                P->paramValues[i] = (char *)x;
                P->paramLengths[i] = strlen(x);
        }
        P->paramFormats[i] = 0;
}


void PostgresqlPreparedStatement_setInt(T P, int parameterIndex, int x) {
        TEST_INDEX
	FREE(P->paramValues[i]);
        P->paramValues[i] = Str_cat("%d", x);
        P->paramLengths[i] = strlen(P->paramValues[i]);
        P->paramFormats[i] = 0;
}


void PostgresqlPreparedStatement_setLLong(T P, int parameterIndex, long long int x) {
        TEST_INDEX
	FREE(P->paramValues[i]);
        P->paramValues[i] = Str_cat("%lld", x);
        P->paramLengths[i] = strlen(P->paramValues[i]);
        P->paramFormats[i] = 0;
}


void PostgresqlPreparedStatement_setDouble(T P, int parameterIndex, double x) {
        TEST_INDEX
	FREE(P->paramValues[i]);
        P->paramValues[i] = Str_cat("%lf", x);
        P->paramLengths[i] = strlen(P->paramValues[i]);
        P->paramFormats[i] = 0;
}


void PostgresqlPreparedStatement_setBlob(T P, int parameterIndex, const void *x, int size) {
        TEST_INDEX
        if (x==NULL) {
                P->paramValues[i] = NULL;
                P->paramLengths[i] = 0;
        } else {
                P->paramValues[i] = (char *)x;
                P->paramLengths[i] = size;
        }
        P->paramFormats[i] = 1;
}


void PostgresqlPreparedStatement_execute(T P) {
        assert(P);
        PQclear(P->res);
        P->res = PQexecPrepared(P->db,
                             P->stmt,
                             P->paramCount,
                             (const char **)P->paramValues,
                             P->paramLengths,
                             P->paramFormats,
                             0);
        P->lastError = PQresultStatus(P->res);
        if (P->lastError != PGRES_COMMAND_OK)
                THROW(SQLException, "Prepared statement failed -- %s", PQresultErrorMessage(P->res));
}


ResultSet_T PostgresqlPreparedStatement_executeQuery(T P) {
        assert(P);
        PQclear(P->res);
        P->res = PQexecPrepared(P->db,
                             P->stmt,
                             P->paramCount,
                             (const char **)P->paramValues,
                             P->paramLengths,
                             P->paramFormats,
                             0);
        P->lastError = PQresultStatus(P->res);
        if (P->lastError == PGRES_TUPLES_OK)
                return ResultSet_new(PostgresqlResultSet_new(P->res, P->maxRows, true), (Rop_T)&postgresqlrops);
        return NULL;
}


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif

