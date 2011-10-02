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
#include <stdarg.h>

#include "URL.h"
#include "Vector.h"
#include "ResultSet.h"
#include "PreparedStatement.h"
#include "Connection.h"
#include "ConnectionPool.h"
#include "ConnectionDelegate.h"


/**
 * Implementation of the Connection interface 
 *
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#ifdef HAVE_LIBMYSQLCLIENT
extern const struct Cop_T mysqlcops;
#endif
#ifdef HAVE_LIBPQ
extern const struct Cop_T postgresqlcops;
#endif
#ifdef HAVE_LIBSQLITE3
extern const struct Cop_T sqlite3cops;
#endif
#ifdef HAVE_ORACLE
extern const struct Cop_T oraclesqlcops;
#endif

static const struct Cop_T *cops[] = {
#ifdef HAVE_LIBMYSQLCLIENT
        &mysqlcops,
#endif
#ifdef HAVE_LIBPQ
        &postgresqlcops,
#endif
#ifdef HAVE_LIBSQLITE3
        &sqlite3cops,
#endif
#ifdef HAVE_ORACLE
        &oraclesqlcops,
#endif
        NULL
};

#define T Connection_T
struct T {
        Cop_T op;
        URL_T url;
	int maxRows;
	int timeout;
	int isAvailable;
        Vector_T prepared;
	int isInTransaction;
        long lastAccessedTime;
        ResultSet_T resultSet;
        ConnectionDelegate_T D;
        ConnectionPool_T parent;
};


/* ------------------------------------------------------- Private methods */


static Cop_T getOp(const char *protocol) {
        for (int i = 0; cops[i]; i++) 
                if (Str_startsWith(protocol, cops[i]->name)) 
                        return (Cop_T)cops[i];
        return NULL;
}


static int setDelegate(T C, char **error) {
        const char *protocol = URL_getProtocol(C->url);
        C->op = getOp(protocol);
        if (! C->op) {
                *error = Str_cat("database protocol '%s' not supported", protocol);
                return false;
        }
        C->D = C->op->new(C->url, error);
        return (C->D != NULL);
}


static void freePrepared(T C) {
	PreparedStatement_T ps;
        while (! Vector_isEmpty(C->prepared)) {
		ps = Vector_pop(C->prepared);
		PreparedStatement_free(&ps);
	}
        assert(Vector_isEmpty(C->prepared));
}


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T Connection_new(void *pool, char **error) {
        T C;
        assert(pool);
	NEW(C);
        C->parent = pool;
	C->isAvailable = true;
	C->isInTransaction = false;
        C->prepared = Vector_new(4);
        C->timeout = SQL_DEFAULT_TIMEOUT;
        C->url = ConnectionPool_getURL(pool);
        C->lastAccessedTime = Util_seconds();
        if (! setDelegate(C, error))
                Connection_free(&C);
	return C;
}


void Connection_free(T *C) {
        assert(C && *C);
        Connection_clear((*C));
        Vector_free(&(*C)->prepared);
        if ((*C)->D)
                (*C)->op->free(&(*C)->D);
	FREE(*C);
}


void Connection_setAvailable(T C, int isAvailable) {
        assert(C);
        C->isAvailable = isAvailable;
        C->lastAccessedTime = Util_seconds();
}


int Connection_isAvailable(T C) {
        assert(C);
        return C->isAvailable;
}


long Connection_getLastAccessedTime(T C) {
        assert(C);
        return C->lastAccessedTime;
}


int Connection_isInTransaction(T C) {
        assert(C);
        return (C->isInTransaction > 0);
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif


/* ------------------------------------------------------------ Properties */


void Connection_setQueryTimeout(T C, int ms) {
        assert(C);
        assert(ms >= 0);
        C->timeout = ms;
        C->op->setQueryTimeout(C->D, ms);
}


int Connection_getQueryTimeout(T C) {
        assert(C);
        return C->timeout;
}


void Connection_setMaxRows(T C, int max) {
        assert(C);
	C->maxRows = max;
        C->op->setMaxRows(C->D, max);
}


int Connection_getMaxRows(T C) {
	assert(C);
	return C->maxRows;
}


URL_T Connection_getURL(T C) {
        assert(C);
        return C->url;
}


/* -------------------------------------------------------- Public methods */


int Connection_ping(T C) {
        assert(C);
        return C->op->ping(C->D);
}


void Connection_clear(T C) {
        assert(C);
        if (C->resultSet)
                ResultSet_free(&C->resultSet);
        if (C->maxRows)
                Connection_setMaxRows(C, 0);
        if (C->timeout != SQL_DEFAULT_TIMEOUT)
                Connection_setQueryTimeout(C, SQL_DEFAULT_TIMEOUT);
        freePrepared(C);
}


void Connection_close(T C) {
        assert(C);
        ConnectionPool_returnConnection(C->parent, C);
}


void Connection_beginTransaction(T C) {
        assert(C);
        if (! C->op->beginTransaction(C->D)) 
                THROW(SQLException, Connection_getLastError(C));
        C->isInTransaction++;
}


void Connection_commit(T C) {
        assert(C);
        if (C->isInTransaction)
                C->isInTransaction = 0;
        // Even if we are not in a transaction, call the delegate anyway and propagate any errors
        if (! C->op->commit(C->D)) 
                THROW(SQLException, Connection_getLastError(C));
}


void Connection_rollback(T C) {
        assert(C);
        if (C->isInTransaction) {
                // Clear any pending resultset statements first
                Connection_clear(C);
                C->isInTransaction = 0;
        }
        // Even if we are not in a transaction, call the delegate anyway and propagate any errors
        if (! C->op->rollback(C->D))
                THROW(SQLException, Connection_getLastError(C));
}


long long int Connection_lastRowId(T C) {
        assert(C);
        return C->op->lastRowId(C->D);
}


long long int Connection_rowsChanged(T C) {
        assert(C);
        return C->op->rowsChanged(C->D);
}


void Connection_execute(T C, const char *sql, ...) {
        int rv = false;
        va_list ap;
        assert(C);
        assert(sql);
        if (C->resultSet)
                ResultSet_free(&C->resultSet);
	va_start(ap, sql);
        rv = C->op->execute(C->D, sql, ap);
        va_end(ap);
        if (rv == false) THROW(SQLException, Connection_getLastError(C));
}


ResultSet_T Connection_executeQuery(T C, const char *sql, ...) {
        va_list ap;
        assert(C);
        assert(sql);
        if (C->resultSet)
                ResultSet_free(&C->resultSet);
	va_start(ap, sql);
        C->resultSet = C->op->executeQuery(C->D, sql, ap);
        va_end(ap);
        if (! C->resultSet)
                THROW(SQLException, Connection_getLastError(C));
        return C->resultSet;
}


PreparedStatement_T Connection_prepareStatement(T C, const char *sql, ...) {
        va_list ap;
        PreparedStatement_T p;
        assert(C);
        assert(sql);
        va_start(ap, sql);
        p = C->op->prepareStatement(C->D, sql, ap);
        va_end(ap);
        if (p)
                Vector_push(C->prepared, p);
        else
                THROW(SQLException, Connection_getLastError(C));
        return p;
}


const char *Connection_getLastError(T C) {
        const char *s;
	assert(C);
	s = C->op->getLastError(C->D);
        return (s ? s : "?");
}


int Connection_isSupported(const char *url) {
        return (url ? (getOp(url) != NULL) : false);
}

