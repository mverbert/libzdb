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
#include <stdatomic.h>

#include "PostgresqlAdapter.h"
#include "StringBuffer.h"
#include "ConnectionDelegate.h"


/**
 * Implementation of the Connection/Delegate interface for postgresql. 
 * 
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#define T ConnectionDelegate_T
struct T {
	PGconn *db;
        PGresult *res;
        StringBuffer_T sb;
        Connection_T delegator;
	ExecStatusType lastError;
};
static _Atomic(uint32_t) kStatementID = 0;
extern const struct Rop_T postgresqlrops;
extern const struct Pop_T postgresqlpops;


/* ------------------------------------------------------- Private methods */


static bool _doConnect(T C, char **error) {
#define ERROR(e) do {*error = Str_dup(e); goto error;} while (0)
        URL_T url = Connection_getURL(C->delegator);
        /* User */
        if (URL_getUser(url))
                StringBuffer_append(C->sb, "user='%s' ", URL_getUser(url));
        else if (URL_getParameter(url, "user"))
                StringBuffer_append(C->sb, "user='%s' ", URL_getParameter(url, "user"));
        else
                ERROR("no username specified in URL");
        /* Password */
        if (URL_getPassword(url))
                StringBuffer_append(C->sb, "password='%s' ", URL_getPassword(url));
        else if (URL_getParameter(url, "password"))
                StringBuffer_append(C->sb, "password='%s' ", URL_getParameter(url, "password"));
        else if (! URL_getParameter(url, "unix-socket"))
                ERROR("no password specified in URL");
        /* Host */
        if (URL_getParameter(url, "unix-socket")) {
                if (URL_getParameter(url, "unix-socket")[0] != '/')
                        ERROR("invalid unix-socket directory");
                StringBuffer_append(C->sb, "host='%s' ", URL_getParameter(url, "unix-socket"));
        } else if (URL_getHost(url)) {
                StringBuffer_append(C->sb, "host='%s' ", URL_getHost(url));
                /* Port */
                if (URL_getPort(url) > 0)
                        StringBuffer_append(C->sb, "port=%d ", URL_getPort(url));
                else
                        ERROR("no port specified in URL");
        } else
                ERROR("no host specified in URL");
        /* Database name */
        if (URL_getPath(url))
                StringBuffer_append(C->sb, "dbname='%s' ", URL_getPath(url) + 1);
        else
                ERROR("no database specified in URL");
        /* Options */
        StringBuffer_append(C->sb, "sslmode='%s' ", IS(URL_getParameter(url, "use-ssl"), "true") ? "require" : "disable");
        if (URL_getParameter(url, "connect-timeout")) {
                TRY
                        StringBuffer_append(C->sb, "connect_timeout=%d ", Str_parseInt(URL_getParameter(url, "connect-timeout")));
                ELSE
                        ERROR("invalid connect timeout value");
                END_TRY;
        } else
                StringBuffer_append(C->sb, "connect_timeout=%d ", SQL_DEFAULT_TIMEOUT/MSEC_PER_SEC);
        if (URL_getParameter(url, "application-name"))
                StringBuffer_append(C->sb, "application_name='%s' ", URL_getParameter(url, "application-name"));
        /* Connect */
        C->db = PQconnectdb(StringBuffer_toString(C->sb));
        if (PQstatus(C->db) == CONNECTION_OK)
                return true;
        *error = Str_dup(PQerrorMessage(C->db));
error:
        return false;
}


/* -------------------------------------------------------- Delegate Methods */


static void _free(T *C) {
        assert(C && *C);
        if ((*C)->res)
                PQclear((*C)->res);
        if ((*C)->db)
                PQfinish((*C)->db);
        StringBuffer_free(&((*C)->sb));
        FREE(*C);
}


static T _new(Connection_T delegator, char **error) {
	T C;
	assert(delegator);
        assert(error);
        NEW(C);
        C->delegator = delegator;
        C->sb = StringBuffer_create(STRLEN);
        if (! _doConnect(C, error))
                _free(&C);
	return C;
}


static bool _ping(T C) {
        assert(C);
        return (PQstatus(C->db) == CONNECTION_OK);
}


static void _setQueryTimeout(T C, int ms) {
        assert(C);
        StringBuffer_set(C->sb, "SET statement_timeout TO %d;", ms);
        PQclear(PQexec(C->db, StringBuffer_toString(C->sb)));
}


static bool _beginTransaction(T C) {
	assert(C);
        PGresult *res = PQexec(C->db, "BEGIN TRANSACTION;");
        C->lastError = PQresultStatus(res);
        PQclear(res);
        return (C->lastError == PGRES_COMMAND_OK);
}


static bool _commit(T C) {
	assert(C);
        PGresult *res = PQexec(C->db, "COMMIT TRANSACTION;");
        C->lastError = PQresultStatus(res);
        PQclear(res);
        return (C->lastError == PGRES_COMMAND_OK);
}


static bool _rollback(T C) {
	assert(C);
        PGresult *res = PQexec(C->db, "ROLLBACK TRANSACTION;");
        C->lastError = PQresultStatus(res);
        PQclear(res);
        return (C->lastError == PGRES_COMMAND_OK);
}


static long long _lastRowId(T C) {
        assert(C);
        return (long long)PQoidValue(C->res);
}


static long long _rowsChanged(T C) {
        assert(C);
        char *changes = PQcmdTuples(C->res);
        return changes ? Str_parseLLong(changes) : 0;
}


static bool _execute(T C, const char *sql, va_list ap) {
	assert(C);
        PQclear(C->res);
        va_list ap_copy;
        va_copy(ap_copy, ap);
        StringBuffer_vset(C->sb, sql, ap_copy);
        va_end(ap_copy);
        C->res = PQexec(C->db, StringBuffer_toString(C->sb));
        C->lastError = PQresultStatus(C->res);
        return (C->lastError == PGRES_COMMAND_OK);
}


static ResultSet_T _executeQuery(T C, const char *sql, va_list ap) {
	assert(C);
        PQclear(C->res);
        va_list ap_copy;
        va_copy(ap_copy, ap);
        StringBuffer_vset(C->sb, sql, ap_copy);
        va_end(ap_copy);
        C->res = PQexec(C->db, StringBuffer_toString(C->sb));
        C->lastError = PQresultStatus(C->res);
        if (C->lastError == PGRES_TUPLES_OK)
                return ResultSet_new(PostgresqlResultSet_new(C->delegator, C->res), (Rop_T)&postgresqlrops);
        return NULL;
}


static PreparedStatement_T _prepareStatement(T C, const char *sql, va_list ap) {
        assert(C);
        assert(sql);
        PQclear(C->res);
        va_list ap_copy;
        va_copy(ap_copy, ap);
        StringBuffer_vset(C->sb, sql, ap_copy);
        va_end(ap_copy);
        int paramCount = StringBuffer_prepare4postgres(C->sb);
        uint32_t t = kStatementID++; // increment is atomic
        char *name = Str_cat("__libzdb-%d", t);
        C->res = PQprepare(C->db, name, StringBuffer_toString(C->sb), 0, NULL);
        C->lastError = C->res ? PQresultStatus(C->res) : PGRES_FATAL_ERROR;
        if (C->lastError == PGRES_EMPTY_QUERY || C->lastError == PGRES_COMMAND_OK || C->lastError == PGRES_TUPLES_OK)
		return PreparedStatement_new(PostgresqlPreparedStatement_new(C->delegator, C->db, name, paramCount), (Pop_T)&postgresqlpops);
        return NULL;
}


static const char *_getLastError(T C) {
	assert(C);
        return C->res ? PQresultErrorMessage(C->res) : "unknown error";
}


/* ------------------------------------------------------------------------- */


const struct Cop_T postgresqlcops = {
        .name             = "postgresql",
        .new              = _new,
        .free             = _free,
        .ping             = _ping,
        .setQueryTimeout  = _setQueryTimeout,
        .beginTransaction = _beginTransaction,
        .commit           = _commit,
        .rollback         = _rollback,
        .lastRowId        = _lastRowId,
        .rowsChanged      = _rowsChanged,
        .execute          = _execute,
        .executeQuery     = _executeQuery,
        .prepareStatement = _prepareStatement,
        .getLastError     = _getLastError
};

