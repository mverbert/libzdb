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
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>

#include "URL.h"
#include "ResultSet.h"
#include "StringBuffer.h"
#include "PreparedStatement.h"
#include "PostgresqlResultSet.h"
#include "PostgresqlPreparedStatement.h"
#include "ConnectionStrategy.h"
#include "PostgresqlConnection.h"


/**
 * Implementation of the Connection/Strategy interface for postgresql. 
 * 
 * @file
 */


/* ----------------------------------------------------------- Definitions */


const struct Cop_T postgresqlcops = {
        "postgresql",
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

#define T ConnectionImpl_T
struct T {
        URL_T url;
	PGconn *db;
	PGresult *res;
	int maxRows;
	int timeout;
	ExecStatusType lastError;
        StringBuffer_T sb;
};

extern const struct Rop_T postgresqlrops;
extern const struct Pop_T postgresqlpops;


/* ------------------------------------------------------------ Prototypes */


static PGconn *doConnect(URL_T url, char **error);


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif


T PostgresqlConnection_new(URL_T url, char **error) {
	T C;
        PGconn *db;
	assert(url);
        assert(error);
        if (! (db = doConnect(url, error)))
                return NULL;
	NEW(C);
        C->db = db;
        C->res = NULL;
        C->url = url;
        C->sb = StringBuffer_new("");
        C->timeout = SQL_DEFAULT_TIMEOUT;
	return C;
}


void PostgresqlConnection_free(T *C) {
	assert(C && *C);
        PQclear((*C)->res);
        PQfinish((*C)->db);
        StringBuffer_free(&(*C)->sb);
	FREE(*C);
}


void PostgresqlConnection_setQueryTimeout(T C, int ms) {
	assert(C);
        C->timeout = ms;
}


void PostgresqlConnection_setMaxRows(T C, int max) {
	assert(C);
        C->maxRows = max;
}


int PostgresqlConnection_ping(T C) {
        assert(C);
        return (PQstatus(C->db)==CONNECTION_OK);
}



int PostgresqlConnection_beginTransaction(T C) {
        PGresult *res;
	assert(C);
        res = PQexec(C->db, "BEGIN TRANSACTION;");
        C->lastError = PQresultStatus(res);
        PQclear(res);
        return (C->lastError == PGRES_COMMAND_OK);
}


int PostgresqlConnection_commit(T C) {
        PGresult *res;
	assert(C);
        res = PQexec(C->db, "COMMIT TRANSACTION;");
        C->lastError = PQresultStatus(res);
        PQclear(res);
        return (C->lastError == PGRES_COMMAND_OK);
}


int PostgresqlConnection_rollback(T C) {
        PGresult *res;
	assert(C);
        res = PQexec(C->db, "ROLLBACK TRANSACTION;");
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
        StringBuffer_clear(C->sb);
        va_copy(ap_copy, ap);
        StringBuffer_vappend(C->sb, sql, ap_copy);
        va_end(ap_copy);
        PQclear(C->res);
        C->res = PQexec(C->db, StringBuffer_toString(C->sb));
        C->lastError = PQresultStatus(C->res);
        if (C->lastError == PGRES_COMMAND_OK)
                return true;
        else {
                THROW(SQLException, "%s", PostgresqlConnection_getLastError(C));
                return false;
        }
}


ResultSet_T PostgresqlConnection_executeQuery(T C, const char *sql, va_list ap) {
        va_list ap_copy;
	assert(C);
        StringBuffer_clear(C->sb);
        va_copy(ap_copy, ap);
        StringBuffer_vappend(C->sb, sql, ap_copy);
        va_end(ap_copy);
        PQclear(C->res);
        C->res = PQexec(C->db, StringBuffer_toString(C->sb));
        C->lastError = PQresultStatus(C->res);
        if (C->lastError == PGRES_TUPLES_OK) {
                return ResultSet_new(PostgresqlResultSet_new(C->res, C->maxRows, false), (Rop_T)&postgresqlrops);
        }
        THROW(SQLException, "%s", PostgresqlConnection_getLastError(C));
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
        /* Thanks PostgreSQL for not using '?' as the wildcard marker, but instead $1, $2.. $n */
        paramCount = StringBuffer_prepare4postgres(C->sb);
        name = Str_cat("%d", rand());
        C->res = PQprepare(C->db, name, StringBuffer_toString(C->sb), 0, NULL);
        if (C->res &&
            (C->lastError == PGRES_EMPTY_QUERY ||
             C->lastError == PGRES_COMMAND_OK ||
             C->lastError == PGRES_TUPLES_OK)) {
		return PreparedStatement_new(PostgresqlPreparedStatement_new(C->db,
                                                                             C->maxRows,
                                                                             name,
                                                                             paramCount), 
                                             (Pop_T)&postgresqlpops);
        }
        THROW(SQLException, "%s", PostgresqlConnection_getLastError(C));
        return NULL;
}


const char *PostgresqlConnection_getLastError(T C) {
	assert(C);
        return C->res ? PQresultErrorMessage(C->res) : "unknown error";
}


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif


/* ------------------------------------------------------- Private methods */


static PGconn *doConnect(URL_T url, char **error) {
#define ERROR(e) do {*error = Str_dup(e); goto error;} while (0)
        int port;
        int ssl = false;
        int connectTimeout = SQL_DEFAULT_TCP_TIMEOUT;
        const char *user, *password, *host, *database, *timeout;
        char *conninfo;
        PGconn *db = NULL;
        if (! (user = URL_getUser(url)))
                if (! (user = URL_getParameter(url, "user")))
                        ERROR("no username specified in URL");
        if (! (password = URL_getPassword(url)))
                if (! (password = URL_getParameter(url, "password")))
                        ERROR("no password specified in URL");
        if (! (host = URL_getHost(url)))
                ERROR("no host specified in URL");
        if ((port = URL_getPort(url))<=0)
                ERROR("no port specified in URL");
        if (! (database = URL_getPath(url)))
                ERROR("no database specified in URL");
        else
                database++;
        if (IS(URL_getParameter(url, "use-ssl"), "true"))
                ssl = true;
        if ((timeout = URL_getParameter(url, "connect-timeout"))) {
                TRY connectTimeout = Str_parseInt(timeout); ELSE ERROR("invalid connect timeout value"); END_TRY;
        }
        conninfo = Str_cat(" host='%s'"
                           " port=%d"
                           " dbname='%s'"
                           " user='%s'"
                           " password='%s'"
                           " connect_timeout=%d"
                           " sslmode='%s'",
                           host,
                           port,
                           database,
                           user,
                           password,
                           connectTimeout,
                           ssl?"require":"disable"
                           );
        db = PQconnectdb(conninfo);
        FREE(conninfo);
        if (PQstatus(db) == CONNECTION_OK)
                return db;
        *error = Str_cat("%s", PQerrorMessage(db));
error:
        PQfinish(db);
        return NULL;
}


