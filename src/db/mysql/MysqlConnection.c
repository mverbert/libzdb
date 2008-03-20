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
#include <string.h>
#include <mysql.h>
#include <errmsg.h>

#include "URL.h"
#include "Str.h"
#include "Mem.h"
#include "Util.h"
#include "ResultSet.h"
#include "StringBuffer.h"
#include "PreparedStatement.h"
#include "MysqlResultSet.h"
#include "MysqlPreparedStatement.h"
#include "ConnectionStrategy.h"
#include "MysqlConnection.h"


/**
 * Implementation of the Connection/Strategy interface for mysql. 
 * Remember the swedish chef from the muppet show? I think he got 
 * a new job at MySQL AB; Hoobi-dubi-doh mysql chicken-soup! 
 * 
 * @version \$Id: MysqlConnection.c,v 1.55 2008/03/20 11:28:53 hauk Exp $
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#define MYSQL_OK 0

const struct conop mysqlconops = {
        "mysql",
        MysqlConnection_new,
        MysqlConnection_free,
        MysqlConnection_setQueryTimeout,
        MysqlConnection_setMaxRows,
        MysqlConnection_ping,
        MysqlConnection_beginTransaction,
        MysqlConnection_commit,
        MysqlConnection_rollback,
        MysqlConnection_lastRowId,
        MysqlConnection_rowsChanged,
        MysqlConnection_execute,
        MysqlConnection_executeQuery,
        MysqlConnection_prepareStatement,
        MysqlConnection_getLastError
};

#define T IConnection_T
struct T {
        URL_T url;
	MYSQL *db;
	int maxRows;
	int timeout;
	int lastError;
        StringBuffer_T sb;
};

extern const struct rsetop mysqlrsetops;
extern const struct prepop mysqlprepops;


/* ------------------------------------------------------------ Prototypes */


static MYSQL *doConnect(URL_T url, char **error);
static int prepareStmt(T C, const char *sql, int len, MYSQL_STMT **stmt);


/* ----------------------------------------------------- Protected methods */

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T MysqlConnection_new(URL_T url, char **error) {
	T C;
        MYSQL *db;
	assert(url);
        assert(error);
        if (! (db = doConnect(url, error)))
                return NULL;
	NEW(C);
        C->db = db;
        C->url = url;
        C->sb = StringBuffer_new("");
	return C;
}


void MysqlConnection_free(T *C) {
	assert(C && *C);
        mysql_close((*C)->db);
        StringBuffer_free(&(*C)->sb);
	FREE(*C);
}


void MysqlConnection_setQueryTimeout(T C, int ms) {
	assert(C);
        C->timeout = ms;
}


void MysqlConnection_setMaxRows(T C, int max) {
	assert(C);
        C->maxRows = max;
}


int MysqlConnection_ping(T C) {
        assert(C);
        return (mysql_ping(C->db) == 0);
}



int MysqlConnection_beginTransaction(T C) {
	assert(C);
        C->lastError = mysql_query(C->db, "START TRANSACTION;");
        return (C->lastError == MYSQL_OK);
}


int MysqlConnection_commit(T C) {
	assert(C);
        C->lastError = mysql_query(C->db, "COMMIT;");
        return (C->lastError == MYSQL_OK);
}


int MysqlConnection_rollback(T C) {
	assert(C);
        C->lastError = mysql_query(C->db, "ROLLBACK;");
        return (C->lastError == MYSQL_OK);
}


long long int MysqlConnection_lastRowId(T C) {
        assert(C);
        return (long long int)mysql_insert_id(C->db);
}


long long int MysqlConnection_rowsChanged(T C) {
        assert(C);
        return (long long int)mysql_affected_rows(C->db);
}


int MysqlConnection_execute(T C, const char *sql, va_list ap) {
	assert(C);
        StringBuffer_clear(C->sb);
        StringBuffer_vappend(C->sb, sql, ap);
        C->lastError = mysql_real_query(C->db, StringBuffer_toString(C->sb), StringBuffer_length(C->sb));
	return (C->lastError==MYSQL_OK);
}


ResultSet_T MysqlConnection_executeQuery(T C, const char *sql, va_list ap) {
        MYSQL_STMT *stmt = NULL;
	assert(C);
        StringBuffer_clear(C->sb);
        StringBuffer_vappend(C->sb, sql, ap);
        prepareStmt(C, StringBuffer_toString(C->sb), StringBuffer_length(C->sb), &stmt);
        if (stmt) {
#if MYSQL_VERSION_ID >= 50002
                unsigned long cursor = CURSOR_TYPE_READ_ONLY;
                mysql_stmt_attr_set(stmt, STMT_ATTR_CURSOR_TYPE, &cursor);
#endif
                if ((C->lastError = mysql_stmt_execute(stmt)))
                        mysql_stmt_close(stmt);
                else
                        return ResultSet_new(MysqlResultSet_new(stmt, C->maxRows, false), (Rop_T)&mysqlrsetops);
        }
        return NULL;
}


PreparedStatement_T MysqlConnection_prepareStatement(T C, const char *sql) {
        MYSQL_STMT *stmt = NULL;
        assert(C);
        if (prepareStmt(C, sql, strlen(sql), &stmt))
		return PreparedStatement_new(MysqlPreparedStatement_new(stmt, C->maxRows), (Pop_T)&mysqlprepops);
        return NULL;
}


const char *MysqlConnection_getLastError(T C) {
	assert(C);
        return mysql_error(C->db);
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif

/* ------------------------------------------------------- Private methods */


static MYSQL *doConnect(URL_T url, char **error) {
#define ERROR(e) do {*error = Str_dup(e); goto error;} while (0)
        int port;
        my_bool yes = 1;
        my_bool no = 0;
        int connectTimeout = SQL_DEFAULT_TCP_TIMEOUT;
        unsigned long clientFlags = CLIENT_MULTI_STATEMENTS;
        const char *user, *password, *host, *database, *charset, *timeout;
        MYSQL *db = mysql_init(NULL);
        if (NULL==db) {
                *error = Str_dup("unable to allocate mysql handler");
                return NULL;
        } 
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
        if (IS(URL_getParameter(url, "compress"), "true"))
                clientFlags |= CLIENT_COMPRESS;
        if (IS(URL_getParameter(url, "use-ssl"), "true"))
                mysql_ssl_set(db, 0,0,0,0,0);
        if (IS(URL_getParameter(url, "secure-auth"), "true"))
                mysql_options(db, MYSQL_SECURE_AUTH, (const char*)&yes);
        else
                mysql_options(db, MYSQL_SECURE_AUTH, (const char*)&no);
        if ((timeout = URL_getParameter(url, "connect-timeout"))) {
                int e = false;
                TRY
                connectTimeout = Str_parseInt(timeout);
                ELSE
                e = true;
                END_TRY;
                if (e) ERROR("invalid connect timeout value");
        }
        mysql_options(db, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&connectTimeout);
        if ((charset = URL_getParameter(url, "charset")))
                mysql_options(db, MYSQL_SET_CHARSET_NAME, charset);
#if MYSQL_VERSION_ID >= 50013
        mysql_options(db, MYSQL_OPT_RECONNECT, (const char*)&yes);
#endif
        if (! mysql_real_connect(db, host, user, password, database, port, 
                               NULL, clientFlags)) {
                *error = Str_cat("%s", mysql_error(db));
                goto error;
        }
	return db;
error:
        mysql_close(db);
        return NULL;
}


static int prepareStmt(T C, const char *sql, int len, MYSQL_STMT **stmt) {
        if (! (*stmt = mysql_stmt_init(C->db))) {
                DEBUG("mysql_stmt_init -- Out of memory\n");
                C->lastError = CR_OUT_OF_MEMORY;
                return false;
        }
        if ((C->lastError = mysql_stmt_prepare(*stmt, sql, len))) {
                mysql_stmt_close(*stmt);
                *stmt = NULL;
                return false;
        }
        return true;
}

