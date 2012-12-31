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

#include "URL.h"
#include "ResultSet.h"
#include "StringBuffer.h"
#include "PreparedStatement.h"
#include "PostgresqlResultSet.h"
#include "PostgresqlPreparedStatement.h"
#include "ConnectionDelegate.h"
#include "PostgresqlConnection.h"


/**
 * Implementation of the Connection/Delegate interface for postgresql. 
 * 
 * @file
 */


/* ----------------------------------------------------------- Definitions */


const struct Cop_T postgresqlcops = {
        "postgresql",
        PostgresqlConnection_onstop,
        PostgresqlConnection_new,
        PostgresqlConnection_free,
        PostgresqlConnection_setQueryTimeout,
        PostgresqlConnection_setMaxRows,
        PostgresqlConnection_ping,
        PostgresqlConnection_beginTransaction,
        PostgresqlConnection_commit,
        PostgresqlConnection_rollback,
        PostgresqlConnection_lastRowId,
        PostgresqlConnection_rowsChanged,
        PostgresqlConnection_execute,
        PostgresqlConnection_executeQuery,
        PostgresqlConnection_prepareStatement,
        PostgresqlConnection_getLastError
};

#define T ConnectionDelegate_T
struct T {
        URL_T url;
	PGconn *db;
	PGresult *res;
	int maxRows;
	int timeout;
	ExecStatusType lastError;
        StringBuffer_T sb;
};
static uint32_t statementid = 0;
extern const struct Rop_T postgresqlrops;
extern const struct Pop_T postgresqlpops;


/* ------------------------------------------------------- Private methods */


static int doConnect(T C, char **error) {
#define ERROR(e) do {*error = Str_dup(e); goto error;} while (0)
        /* User */
        if (URL_getUser(C->url))
                StringBuffer_append(C->sb, "user='%s' ", URL_getUser(C->url));
        else if (URL_getParameter(C->url, "user"))
                StringBuffer_append(C->sb, "user='%s' ", URL_getParameter(C->url, "user"));
        else
                ERROR("no username specified in URL");
        /* Password */
        if (URL_getPassword(C->url))
                StringBuffer_append(C->sb, "password='%s' ", URL_getPassword(C->url));
        else if (URL_getParameter(C->url, "password"))
                StringBuffer_append(C->sb, "password='%s' ", URL_getParameter(C->url, "password"));
        else
                ERROR("no password specified in URL");
        /* Host */
        if (URL_getParameter(C->url, "unix-socket")) {
                if (URL_getParameter(C->url, "unix-socket")[0] != '/')
                        ERROR("invalid unix-socket directory");
                StringBuffer_append(C->sb, "host='%s' ", URL_getParameter(C->url, "unix-socket"));
        } else if (URL_getHost(C->url)) {
                StringBuffer_append(C->sb, "host='%s' ", URL_getHost(C->url));
                /* Port */
                if (URL_getPort(C->url) > 0)
                        StringBuffer_append(C->sb, "port=%d ", URL_getPort(C->url));
                else
                        ERROR("no port specified in URL");
        } else
                ERROR("no host specified in URL");
        /* Database name */
        if (URL_getPath(C->url))
                StringBuffer_append(C->sb, "dbname='%s' ", URL_getPath(C->url) + 1);
        else
                ERROR("no database specified in URL");
        /* Options */
        StringBuffer_append(C->sb, "sslmode='%s' ", IS(URL_getParameter(C->url, "use-ssl"), "true") ? "require" : "disable");
        if (URL_getParameter(C->url, "connect-timeout")) {
                TRY
                        StringBuffer_append(C->sb, "connect_timeout=%d ", Str_parseInt(URL_getParameter(C->url, "connect-timeout")));
                ELSE
                        ERROR("invalid connect timeout value");
                END_TRY;
        } else
                StringBuffer_append(C->sb, "connect_timeout=%d ", SQL_DEFAULT_TCP_TIMEOUT);
        if (URL_getParameter(C->url, "application-name"))
                StringBuffer_append(C->sb, "application_name='%s' ", URL_getParameter(C->url, "application-name"));
        /* Connect */
        C->db = PQconnectdb(StringBuffer_toString(C->sb));
        if (PQstatus(C->db) == CONNECTION_OK)
                return true;
        *error = Str_dup(PQerrorMessage(C->db));
error:
        return false;
}


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T PostgresqlConnection_new(URL_T url, char **error) {
	T C;
	assert(url);
        assert(error);
        NEW(C);
        C->url = url;
        C->sb = StringBuffer_create(STRLEN);
        C->timeout = SQL_DEFAULT_TIMEOUT;
        if (! doConnect(C, error))
                PostgresqlConnection_free(&C);
	return C;
}


void PostgresqlConnection_free(T *C) {
	assert(C && *C);
        if ((*C)->res)
                PQclear((*C)->res);
        if ((*C)->db)
                PQfinish((*C)->db);
        StringBuffer_free(&(*C)->sb);
	FREE(*C);
}


void PostgresqlConnection_setQueryTimeout(T C, int ms) {
	assert(C);
        C->timeout = ms;
        StringBuffer_clear(C->sb);
        StringBuffer_append(C->sb, "SET statement_timeout TO %d;", C->timeout);
        PGresult *res = PQexec(C->db, StringBuffer_toString(C->sb));
        PQclear(res);
}


void PostgresqlConnection_setMaxRows(T C, int max) {
	assert(C);
        C->maxRows = max;
}


int PostgresqlConnection_ping(T C) {
        assert(C);
        return (PQstatus(C->db) == CONNECTION_OK);
}



int PostgresqlConnection_beginTransaction(T C) {
	assert(C);
        PGresult *res = PQexec(C->db, "BEGIN TRANSACTION;");
        C->lastError = PQresultStatus(res);
        PQclear(res);
        return (C->lastError == PGRES_COMMAND_OK);
}


int PostgresqlConnection_commit(T C) {
	assert(C);
        PGresult *res = PQexec(C->db, "COMMIT TRANSACTION;");
        C->lastError = PQresultStatus(res);
        PQclear(res);
        return (C->lastError == PGRES_COMMAND_OK);
}


int PostgresqlConnection_rollback(T C) {
	assert(C);
        PGresult *res = PQexec(C->db, "ROLLBACK TRANSACTION;");
        C->lastError = PQresultStatus(res);
        PQclear(res);
        return (C->lastError == PGRES_COMMAND_OK);
}


long long int PostgresqlConnection_lastRowId(T C) {
        assert(C);
        return (long long int)PQoidValue(C->res);
}


long long int PostgresqlConnection_rowsChanged(T C) {
        assert(C);
        return Str_parseLLong(PQcmdTuples(C->res));
}


int PostgresqlConnection_execute(T C, const char *sql, va_list ap) {
        va_list ap_copy;
	assert(C);
        PQclear(C->res);
        StringBuffer_clear(C->sb);
        va_copy(ap_copy, ap);
        StringBuffer_vappend(C->sb, sql, ap_copy);
        va_end(ap_copy);
        C->res = PQexec(C->db, StringBuffer_toString(C->sb));
        C->lastError = PQresultStatus(C->res);
        return (C->lastError == PGRES_COMMAND_OK);
}


ResultSet_T PostgresqlConnection_executeQuery(T C, const char *sql, va_list ap) {
        va_list ap_copy;
	assert(C);
        PQclear(C->res);
        StringBuffer_clear(C->sb);
        va_copy(ap_copy, ap);
        StringBuffer_vappend(C->sb, sql, ap_copy);
        va_end(ap_copy);
        C->res = PQexec(C->db, StringBuffer_toString(C->sb));
        C->lastError = PQresultStatus(C->res);
        if (C->lastError == PGRES_TUPLES_OK)
                return ResultSet_new(PostgresqlResultSet_new(C->res, C->maxRows), (Rop_T)&postgresqlrops);
        return NULL;
}


PreparedStatement_T PostgresqlConnection_prepareStatement(T C, const char *sql, va_list ap) {
        char *name;
        int paramCount = 0;
        va_list ap_copy;
        assert(C);
        assert(sql);
        PQclear(C->res);
        StringBuffer_clear(C->sb);
        va_copy(ap_copy, ap);
        StringBuffer_vappend(C->sb, sql, ap_copy);
        va_end(ap_copy);
        paramCount = StringBuffer_prepare4postgres(C->sb);
        uint32_t t = ++statementid; // increment is atomic
        name = Str_cat("%d", t);
        C->res = PQprepare(C->db, name, StringBuffer_toString(C->sb), 0, NULL);
        if (C->res && (C->lastError == PGRES_EMPTY_QUERY || C->lastError == PGRES_COMMAND_OK || C->lastError == PGRES_TUPLES_OK))
		return PreparedStatement_new(PostgresqlPreparedStatement_new(C->db, C->maxRows, name, paramCount), (Pop_T)&postgresqlpops);
        return NULL;
}


const char *PostgresqlConnection_getLastError(T C) {
	assert(C);
        return C->res ? PQresultErrorMessage(C->res) : "unknown error";
}

/* Postgres client library finalization */
void  PostgresqlConnection_onstop(void) {
        // Not needed, PostgresqlConnection_free below handle finalization
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif

