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
#include <string.h>
#include <libpq-fe.h>

#include "ResultSet.h"
#include "PostgresqlResultSet.h"
#include "PreparedStatementDelegate.h"
#include "PostgresqlPreparedStatement.h"


/**
 * Implementation of the PreparedStatement/Delegate interface for postgresql.
 * All parameter values are sent as text except for blobs. Postgres ignore
 * paramLengths for text parameters and it is therefor set to 0, except for blob.
 *
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

typedef struct param_t {
        char s[65];
} *param_t;
#define T PreparedStatementDelegate_T
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
        param_t params;
};

#define TEST_INDEX \
        int i; assert(P); i = parameterIndex - 1; if (P->paramCount <= 0 || \
        i < 0 || i >= P->paramCount) THROW(SQLException, "Parameter index is out of range");

extern const struct Rop_T postgresqlrops;


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T PostgresqlPreparedStatement_new(PGconn *db, int maxRows, char *stmt, int paramCount) {
        T P;
        assert(db);
        assert(stmt);
        NEW(P);
        P->db = db;
        P->stmt = stmt;
        P->maxRows = maxRows;
        P->paramCount = paramCount;
        P->lastError = PGRES_COMMAND_OK;
        if (P->paramCount) {
                P->paramValues = CALLOC(P->paramCount, sizeof(char *));
                P->paramLengths = CALLOC(P->paramCount, sizeof(int));
                P->paramFormats = CALLOC(P->paramCount, sizeof(int));
                P->params = CALLOC(P->paramCount, sizeof(struct param_t));
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
	        FREE((*P)->paramValues);
	        FREE((*P)->paramLengths);
	        FREE((*P)->paramFormats);
	        FREE((*P)->params);
        }
	FREE(*P);
}


void PostgresqlPreparedStatement_setString(T P, int parameterIndex, const char *x) {
        TEST_INDEX
        P->paramValues[i] = (char *)x;
        P->paramLengths[i] = 0;
        P->paramFormats[i] = 0;
}


void PostgresqlPreparedStatement_setInt(T P, int parameterIndex, int x) {
        TEST_INDEX
        snprintf(P->params[i].s, 64, "%d", x);
        P->paramValues[i] =  P->params[i].s;
        P->paramLengths[i] = 0;
        P->paramFormats[i] = 0;
}


void PostgresqlPreparedStatement_setLLong(T P, int parameterIndex, long long int x) {
        TEST_INDEX
        snprintf(P->params[i].s, 64, "%lld", x);
        P->paramValues[i] =  P->params[i].s;
        P->paramLengths[i] = 0; 
        P->paramFormats[i] = 0;
}


void PostgresqlPreparedStatement_setDouble(T P, int parameterIndex, double x) {
        TEST_INDEX
        snprintf(P->params[i].s, 64, "%lf", x);
        P->paramValues[i] =  P->params[i].s;
        P->paramLengths[i] = 0;
        P->paramFormats[i] = 0;
}


void PostgresqlPreparedStatement_setBlob(T P, int parameterIndex, const void *x, int size) {
        TEST_INDEX
        P->paramValues[i] = (char *)x;
        P->paramLengths[i] = (x) ? size : 0;
        P->paramFormats[i] = 1;
}


void PostgresqlPreparedStatement_execute(T P) {
        assert(P);
        PQclear(P->res);
        P->res = PQexecPrepared(P->db, P->stmt, P->paramCount, (const char **)P->paramValues, P->paramLengths, P->paramFormats, 0);
        P->lastError = PQresultStatus(P->res);
        if (P->lastError != PGRES_COMMAND_OK)
                THROW(SQLException, "%s", PQresultErrorMessage(P->res));
}


ResultSet_T PostgresqlPreparedStatement_executeQuery(T P) {
        assert(P);
        PQclear(P->res);
        P->res = PQexecPrepared(P->db, P->stmt, P->paramCount, (const char **)P->paramValues, P->paramLengths, P->paramFormats, 0);
        P->lastError = PQresultStatus(P->res);
        if (P->lastError == PGRES_TUPLES_OK)
                return ResultSet_new(PostgresqlResultSet_new(P->res, P->maxRows), (Rop_T)&postgresqlrops);
        THROW(SQLException, "%s", PQresultErrorMessage(P->res));
        return NULL;
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif

