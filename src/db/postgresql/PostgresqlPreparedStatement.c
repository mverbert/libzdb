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
#include <string.h>
#include <libpq-fe.h>

#include "system/Time.h"
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
        .name           = "postgresql",
        .free           = PostgresqlPreparedStatement_free,
        .setString      = PostgresqlPreparedStatement_setString,
        .setInt         = PostgresqlPreparedStatement_setInt,
        .setLLong       = PostgresqlPreparedStatement_setLLong,
        .setDouble      = PostgresqlPreparedStatement_setDouble,
        .setTimestamp   = PostgresqlPreparedStatement_setTimestamp,
        .setBlob        = PostgresqlPreparedStatement_setBlob,
        .execute        = PostgresqlPreparedStatement_execute,
        .executeQuery   = PostgresqlPreparedStatement_executeQuery,
        .rowsChanged    = PostgresqlPreparedStatement_rowsChanged
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
        assert(P);
        int i = checkAndSetParameterIndex(parameterIndex, P->paramCount);
        P->paramValues[i] = (char *)x;
        P->paramLengths[i] = 0;
        P->paramFormats[i] = 0;
}


void PostgresqlPreparedStatement_setInt(T P, int parameterIndex, int x) {
        assert(P);
        int i = checkAndSetParameterIndex(parameterIndex, P->paramCount);
        snprintf(P->params[i].s, 64, "%d", x);
        P->paramValues[i] =  P->params[i].s;
        P->paramLengths[i] = 0;
        P->paramFormats[i] = 0;
}


void PostgresqlPreparedStatement_setLLong(T P, int parameterIndex, long long x) {
        assert(P);
        int i = checkAndSetParameterIndex(parameterIndex, P->paramCount);
        snprintf(P->params[i].s, 64, "%lld", x);
        P->paramValues[i] =  P->params[i].s;
        P->paramLengths[i] = 0; 
        P->paramFormats[i] = 0;
}


void PostgresqlPreparedStatement_setDouble(T P, int parameterIndex, double x) {
        assert(P);
        int i = checkAndSetParameterIndex(parameterIndex, P->paramCount);
        snprintf(P->params[i].s, 64, "%lf", x);
        P->paramValues[i] =  P->params[i].s;
        P->paramLengths[i] = 0;
        P->paramFormats[i] = 0;
}


void PostgresqlPreparedStatement_setTimestamp(T P, int parameterIndex, time_t x) {
        assert(P);
        int i = checkAndSetParameterIndex(parameterIndex, P->paramCount);
        P->paramValues[i] = Time_toString(x, P->params[i].s);
        P->paramLengths[i] = 0;
        P->paramFormats[i] = 0;
}


void PostgresqlPreparedStatement_setBlob(T P, int parameterIndex, const void *x, int size) {
        assert(P);
        int i = checkAndSetParameterIndex(parameterIndex, P->paramCount);
        P->paramValues[i] = (char *)x;
        P->paramLengths[i] = (x) ? size : 0;
        P->paramFormats[i] = 1;
}


void PostgresqlPreparedStatement_execute(T P) {
        assert(P);
        PQclear(P->res);
        P->res = PQexecPrepared(P->db, P->stmt, P->paramCount, (const char **)P->paramValues, P->paramLengths, P->paramFormats, 0);
        P->lastError = P->res ? PQresultStatus(P->res) : PGRES_FATAL_ERROR;
        if (P->lastError != PGRES_COMMAND_OK)
                THROW(SQLException, "%s", PQresultErrorMessage(P->res));
}


ResultSet_T PostgresqlPreparedStatement_executeQuery(T P) {
        assert(P);
        PQclear(P->res);
        P->res = PQexecPrepared(P->db, P->stmt, P->paramCount, (const char **)P->paramValues, P->paramLengths, P->paramFormats, 0);
        P->lastError = P->res ? PQresultStatus(P->res) : PGRES_FATAL_ERROR;
        if (P->lastError == PGRES_TUPLES_OK)
                return ResultSet_new(PostgresqlResultSet_new(P->res, P->maxRows), (Rop_T)&postgresqlrops);
        THROW(SQLException, "%s", PQresultErrorMessage(P->res));
        return NULL;
}


long long PostgresqlPreparedStatement_rowsChanged(T P) {
        assert(P);
        char *changes = PQcmdTuples(P->res);
        return changes ? Str_parseLLong(changes) : 0;
}


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif

