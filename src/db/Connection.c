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
#include <stdarg.h>

#include "URL.h"
#include "Vector.h"
#include "ResultSet.h"
#include "PreparedStatement.h"
#include "Connection.h"
#include "ConnectionPool.h"
#include "ConnectionStrategy.h"


/**
 * Implementation of the Connection interface 
 *
 * @version \$Id: Connection.c,v 1.53 2008/03/20 11:28:53 hauk Exp $
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#ifdef HAVE_LIBMYSQLCLIENT
extern const struct conop mysqlconops;
#endif
#ifdef HAVE_LIBPQ
extern const struct conop postgresqlconops;
#endif
#ifdef HAVE_LIBSQLITE3
extern const struct conop sqlite3conops;
#endif

const struct conop *conops[] = {
#ifdef HAVE_LIBMYSQLCLIENT
        &mysqlconops,
#endif
#ifdef HAVE_LIBPQ
        &postgresqlconops,
#endif
#ifdef HAVE_LIBSQLITE3
        &sqlite3conops,
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
        IConnection_T db;
        Vector_T prepared;
	int isInTransaction;
        long lastAccessedTime;
        ResultSet_T resultSet;
        ConnectionPool_T parent;
};


/* ------------------------------------------------------------ Prototypes */


static void freePrepared(T C);
static Cop_T getOp(const char *protocol);
static int setStrategy(T C, char **error);


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
        C->prepared = Vector_new(1);
        C->timeout = SQL_DEFAULT_TIMEOUT;
        C->url = ConnectionPool_getURL(pool);
        if (! setStrategy(C, error)) {
                Connection_free(&C);
                return NULL;
        }
        C->lastAccessedTime = Util_seconds();
	return C;
}


void Connection_free(T *C) {
        assert(C && *C);
        Connection_clear((*C));
        Vector_free(&(*C)->prepared);
        if ((*C)->db)
                (*C)->op->free(&(*C)->db);
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


/* -------------------------------------------------------- Public methods */


void Connection_setQueryTimeout(T C, int ms) {
        assert(C);
        assert(ms >= 0);
        C->timeout = ms;
        C->op->setQueryTimeout(C->db, ms);
}


int Connection_getQueryTimeout(T C) {
        assert(C);
        return C->timeout;
}


void Connection_setMaxRows(T C, int max) {
        assert(C);
	C->maxRows = max;
        C->op->setMaxRows(C->db, max);
}


int Connection_getMaxRows(T C) {
	assert(C);
	return C->maxRows;
}


URL_T Connection_getURL(T C) {
        assert(C);
        return C->url;
}


void Connection_clear(T C) {
        assert(C);
        if (C->resultSet) {
                ResultSet_free(&C->resultSet);
        }
        freePrepared(C);
}


void Connection_close(T C) {
        assert(C);
        ConnectionPool_returnConnection(C->parent, C);
}


int Connection_ping(T C) {
        assert(C);
        return C->op->ping(C->db);
}


int Connection_beginTransaction(T C) {
        assert(C);
        if (! C->op->beginTransaction(C->db)) {
                THROW(SQLException, Connection_getLastError(C));
                return false;
        }
        C->isInTransaction++;
        return true;
}


int Connection_commit(T C) {
        assert(C);
        if (C->isInTransaction) {
                C->isInTransaction--;
        }
        if (! C->op->commit(C->db)) {
                THROW(SQLException, Connection_getLastError(C));
                return false;
        }
        return true;
}


int Connection_rollback(T C) {
        assert(C);
        if (C->isInTransaction) {
                C->isInTransaction--;
        }
        if (! C->op->rollback(C->db)) {
                THROW(SQLException, Connection_getLastError(C));
                return false;
        }
        return true;
}


long long int Connection_lastRowId(T C) {
        assert(C);
        return C->op->lastRowId(C->db);
}


long long int Connection_rowsChanged(T C) {
        assert(C);
        return C->op->rowsChanged(C->db);
}


int Connection_execute(T C, const char *sql, ...) {
        int rv = false;
        va_list ap;
        assert(C);
        if (C->resultSet)
                ResultSet_free(&C->resultSet);
	va_start(ap, sql);
        rv = C->op->execute(C->db, sql, ap);
        va_end(ap);
        if (rv == false)
                THROW(SQLException, Connection_getLastError(C));
        return rv;
}


ResultSet_T Connection_executeQuery(T C, const char *sql, ...) {
        va_list ap;
        assert(C);
        if (C->resultSet)
                ResultSet_free(&C->resultSet);
	va_start(ap, sql);
        C->resultSet = C->op->executeQuery(C->db, sql, ap);
        va_end(ap);
        if (C->resultSet == NULL)
                THROW(SQLException, Connection_getLastError(C));
        return C->resultSet;
}


PreparedStatement_T Connection_prepareStatement(T C, const char *sql) {
        PreparedStatement_T p;
        assert(C);
        p = C->op->prepareStatement(C->db, sql);
        if (p)
                Vector_push(C->prepared, p);
        else
                THROW(SQLException, Connection_getLastError(C));
        return p;
}


const char *Connection_getLastError(T C) {
        const char *s;
	assert(C);
	s = C->op->getLastError(C->db);
        return (s ? s : "?");
}


int Connection_isSupported(const char *url) {
        return (url ? (getOp(url) != NULL) : false);
}


/* ------------------------------------------------------- Private methods */


static Cop_T getOp(const char *protocol) {
        int i;
        for (i = 0; conops[i]; i++) 
                if (Str_startsWith(conops[i]->name, protocol)) 
                        return (Cop_T)conops[i];
        return NULL;
}


static int setStrategy(T C, char **error) {
        const char *protocol = URL_getProtocol(C->url);
        C->op = getOp(protocol);
        if (C->op == NULL) {
                *error = Str_cat("database protocol '%s' not supported", protocol);
                return false;
        }
        C->db = C->op->new(C->url, error);
        return (C->db != NULL);
}


static void freePrepared(T C) {
	PreparedStatement_T ps;
        while (! Vector_isEmpty(C->prepared)) {
		ps = Vector_pop(C->prepared);
		PreparedStatement_free(&ps);
	}
        assert(Vector_isEmpty(C->prepared));
}

