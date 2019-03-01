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
#include <mysql.h>
#include <errmsg.h>
#include <ctype.h>

#include "zdb.h"
#include "ConnectionDelegate.h"
#include "StringBuffer.h"


/**
 * Implementation of the Connection/Delegate interface for mysql.
 *
 * TODO: Query Timeout has no effect as this is not implemented in MySQL.
 * MySQL does not provide a general way to do this. The MySQL
 * JDBC driver implements query timeout by running a separate timer
 * thread and then KILL the process in the server if query execution
 * time exceed timeout. This is a brute force approach with potential
 * resource side-effects which we'd rather not use.
 *
 * If you use innodb, setting innodb_lock_wait_timeout in the server
 * can be a possible alternative. Default innodb_lock_wait_timeout is 50 sec.
 *
 * @file
 */


/* ------------------------------------------------------------- Definitions */


#define T ConnectionDelegate_T
struct T {
        Connection_T delegator;
        MYSQL *db;
        int lastError;
        StringBuffer_T sb;
};
#define MYSQL_OK 0

extern const struct Rop_T mysqlrops;
extern const struct Pop_T mysqlpops;


/* --------------------------------------------------------- Private methods */


static MYSQL *_doConnect(Connection_T delegator, char **error) {
#define ERROR(e) do {*error = Str_dup(e); goto error;} while (0)
        URL_T url = Connection_getURL(delegator);
        my_bool yes = 1, no = 0;
        int connectTimeout = SQL_DEFAULT_TIMEOUT / MSEC_PER_SEC;
        unsigned long clientFlags = CLIENT_MULTI_STATEMENTS;
        MYSQL *db = mysql_init(NULL);
        if (! db) {
                *error = Str_dup("unable to allocate mysql handler");
                return NULL;
        }
        const char *user = URL_getUser(url);
        if (! user)
                if (! (user = URL_getParameter(url, "user")))
                        ERROR("no username specified in URL");
        const char *password = URL_getPassword(url);
        if (! password)
                if (! (password = URL_getParameter(url, "password")))
                        ERROR("no password specified in URL");
        const char *host = URL_getHost(url);
        const char *unix_socket = URL_getParameter(url, "unix-socket");
        if (unix_socket) {
                host = "localhost"; // Make sure host is localhost if unix socket is to be used
        } else if (! host)
                ERROR("no host specified in URL");
        int port = URL_getPort(url);
        if (port <= 0)
                ERROR("no port specified in URL");
        const char *database = URL_getPath(url);
        if (! database)
                ERROR("no database specified in URL");
        else
                database++;
        // Options
        if (IS(URL_getParameter(url, "compress"), "true"))
                clientFlags |= CLIENT_COMPRESS;
        if (IS(URL_getParameter(url, "use-ssl"), "true"))
                mysql_ssl_set(db, 0,0,0,0,0);
        if (IS(URL_getParameter(url, "secure-auth"), "true"))
                mysql_options(db, MYSQL_SECURE_AUTH, (const char*)&yes);
        else
                mysql_options(db, MYSQL_SECURE_AUTH, (const char*)&no);
        const char *timeout = URL_getParameter(url, "connect-timeout");
        if (timeout)
                connectTimeout = Str_parseInt(timeout);
        mysql_options(db, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&connectTimeout);
        const char *charset = URL_getParameter(url, "charset");
        if (charset)
                mysql_options(db, MYSQL_SET_CHARSET_NAME, charset);
#if MYSQL_VERSION_ID >= 50013
        mysql_options(db, MYSQL_OPT_RECONNECT, (const char*)&yes);
#endif
        // Set Connection ResultSet fetch size if found in URL
        const char *fetchSize = URL_getParameter(url, "fetch-size");
        if (fetchSize) {
                if (!isdigit(fetchSize[0]))
                        ERROR("Invalid fetch-size value. Not a number");
                int rows = Str_parseInt(fetchSize);
                if (rows < 1)
                        ERROR("invalid fetch-size");
                Connection_setFetchSize(delegator, rows);
        }
        // Connect
        if (mysql_real_connect(db, host, user, password, database, port, unix_socket, clientFlags))
                return db;
        *error = Str_dup(mysql_error(db));
error:
        mysql_close(db);
        return NULL;
}


static int _prepare(T C, const char *sql, int len, MYSQL_STMT **stmt) {
        if (! (*stmt = mysql_stmt_init(C->db))) {
                DEBUG("mysql_stmt_init -- Out of memory\n");
                C->lastError = CR_OUT_OF_MEMORY;
                return false;
        }
        if ((C->lastError = mysql_stmt_prepare(*stmt, sql, len))) {
                StringBuffer_set(C->sb, "%s", mysql_stmt_error(*stmt));
                mysql_stmt_close(*stmt);
                *stmt = NULL;
                return false;
        }
        return true;
}


/* ---------------------------------------------- ConnectionDelegate methods */


static T _new(Connection_T delegator, char **error) {
        assert(delegator);
        assert(error);
        T C;
        MYSQL *db;
        if (! (db = _doConnect(delegator, error)))
                return NULL;
        NEW(C);
        C->db = db;
        C->delegator = delegator;
        C->sb = StringBuffer_create(STRLEN);
        return C;
}


static void _free(T *C) {
        assert(C && *C);
        mysql_close((*C)->db);
        StringBuffer_free(&((*C)->sb));
        FREE(*C);
}


static int _ping(T C) {
        assert(C);
        return (mysql_ping(C->db) == 0);
}


static int _beginTransaction(T C) {
        assert(C);
        C->lastError = mysql_query(C->db, "START TRANSACTION;");
        return (C->lastError == MYSQL_OK);
}


static int _commit(T C) {
        assert(C);
        C->lastError = mysql_query(C->db, "COMMIT;");
        return (C->lastError == MYSQL_OK);
}


static int _rollback(T C) {
        assert(C);
        C->lastError = mysql_query(C->db, "ROLLBACK;");
        return (C->lastError == MYSQL_OK);
}


static long long _lastRowId(T C) {
        assert(C);
        return (long long)mysql_insert_id(C->db);
}


static long long _rowsChanged(T C) {
        assert(C);
        return (long long)mysql_affected_rows(C->db);
}


static int _execute(T C, const char *sql, va_list ap) {
        assert(C);
        va_list ap_copy;
        va_copy(ap_copy, ap);
        StringBuffer_vset(C->sb, sql, ap_copy);
        va_end(ap_copy);
        C->lastError = mysql_real_query(C->db, StringBuffer_toString(C->sb), StringBuffer_length(C->sb));
        return (C->lastError == MYSQL_OK);
}


static ResultSet_T _executeQuery(T C, const char *sql, va_list ap) {
        assert(C);
        va_list ap_copy;
        va_copy(ap_copy, ap);
        StringBuffer_vset(C->sb, sql, ap_copy);
        va_end(ap_copy);
        MYSQL_STMT *stmt = NULL;
        if (_prepare(C, StringBuffer_toString(C->sb), StringBuffer_length(C->sb), &stmt)) {
#if MYSQL_VERSION_ID >= 50002
                unsigned long cursor = CURSOR_TYPE_READ_ONLY;
                mysql_stmt_attr_set(stmt, STMT_ATTR_CURSOR_TYPE, &cursor);
#endif
                unsigned long fetchSize = Connection_getFetchSize(C->delegator);
                if ((C->lastError = mysql_stmt_execute(stmt))
                    || (C->lastError = mysql_stmt_attr_set(stmt, STMT_ATTR_PREFETCH_ROWS, &fetchSize))) {
                        StringBuffer_set(C->sb, "%s", mysql_stmt_error(stmt));
                        mysql_stmt_close(stmt);
                } else
                        return ResultSet_new(mysqlrops.new(C->delegator, stmt, false), (Rop_T)&mysqlrops);
        }
        return NULL;
}


static PreparedStatement_T _prepareStatement(T C, const char *sql, va_list ap) {
        assert(C);
        va_list ap_copy;
        va_copy(ap_copy, ap);
        StringBuffer_vset(C->sb, sql, ap_copy);
        va_end(ap_copy);
        MYSQL_STMT *stmt = NULL;
        if (_prepare(C, StringBuffer_toString(C->sb), StringBuffer_length(C->sb), &stmt)) {
                return PreparedStatement_new(mysqlpops.new(C->delegator, stmt), (Pop_T)&mysqlpops);
        }
        return NULL;
}


static const char *_getLastError(T C) {
        assert(C);
        if (mysql_errno(C->db))
                return mysql_error(C->db);
        return StringBuffer_toString(C->sb); // Either the statement itself or a statement error
}


/* ------------------------------------------------ MySQL ConnectionDelegate */


const struct Cop_T mysqlcops = {
        .name 		  = "mysql",
        .new 		  = _new,
        .free 		  = _free,
        .ping		  = _ping,
        .beginTransaction = _beginTransaction,
        .commit		  = _commit,
        .rollback	  = _rollback,
        .lastRowId	  = _lastRowId,
        .rowsChanged	  = _rowsChanged,
        .execute	  = _execute,
        .executeQuery     = _executeQuery,
        .prepareStatement = _prepareStatement,
        .getLastError     = _getLastError
};

