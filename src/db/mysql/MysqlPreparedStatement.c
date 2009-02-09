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
#include <mysql.h>

#include "ResultSet.h"
#include "MysqlResultSet.h"
#include "PreparedStatementStrategy.h"
#include "MysqlPreparedStatement.h"


/**
 * Implementation of the PreparedStatement/Strategy interface for mysql.
 *
 * @version \$Id: MysqlPreparedStatement.c,v 1.31 2008/03/20 11:28:53 hauk Exp $
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#define MYSQL_OK 0

const struct Pop_T mysqlpops = {
        "mysql",
        MysqlPreparedStatement_free,
        MysqlPreparedStatement_setString,
        MysqlPreparedStatement_setInt,
        MysqlPreparedStatement_setLLong,
        MysqlPreparedStatement_setDouble,
        MysqlPreparedStatement_setBlob,
        MysqlPreparedStatement_execute,
        MysqlPreparedStatement_executeQuery
};

typedef struct param_t {
        union {
                long i;
                long long int ll;
                double d;
        };
        long length;
} *param_t;

#define T PreparedStatementImpl_T
struct T {
        int maxRows;
        int lastError;
        int paramCount;
        param_t params;
        MYSQL_STMT *stmt;
        MYSQL_BIND *bind;
};

static my_bool yes = true;

#define TEST_INDEX \
        int i; assert(P); i= parameterIndex - 1; if (P->paramCount <= 0 || \
        i < 0 || i >= P->paramCount) THROW(SQLException, "Parameter index is out of range"); 

extern const struct Rop_T mysqlrops;


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif


T MysqlPreparedStatement_new(void *stmt, int maxRows) {
        T P;
        assert(stmt);
        NEW(P);
        P->stmt = stmt;
        P->maxRows = maxRows;
        P->paramCount = (int)mysql_stmt_param_count(P->stmt);
        if (P->paramCount>0) {
                P->params = CALLOC(P->paramCount, sizeof(struct param_t));
                P->bind = CALLOC(P->paramCount, sizeof(MYSQL_BIND));
        }
        P->lastError = MYSQL_OK;
        return P;
}


void MysqlPreparedStatement_free(T *P) {
	assert(P && *P);
        FREE((*P)->bind);
        mysql_stmt_free_result((*P)->stmt);
        mysql_stmt_close((*P)->stmt);
        FREE((*P)->params);
	FREE(*P);
}


void MysqlPreparedStatement_setString(T P, int parameterIndex, const char *x) {
        TEST_INDEX
        P->bind[i].buffer_type = MYSQL_TYPE_STRING;
        P->bind[i].buffer = (char*)x;
        if (x==NULL) {
                P->params[i].length = 0;
                P->bind[i].is_null = &yes;
        } else {
                P->params[i].length = strlen(x);
                P->bind[i].is_null = 0;
        }
        P->bind[i].length = &P->params[i].length;
}


void MysqlPreparedStatement_setInt(T P, int parameterIndex, int x) {
        TEST_INDEX
        P->params[i].i = x;
        P->bind[i].buffer_type = MYSQL_TYPE_LONG;
        P->bind[i].buffer = (char*)&P->params[i].i;
        P->bind[i].is_null = 0;
}


void MysqlPreparedStatement_setLLong(T P, int parameterIndex, long long int x) {
        TEST_INDEX
        P->params[i].ll = x;
        P->bind[i].buffer_type = MYSQL_TYPE_LONGLONG;
        P->bind[i].buffer = (char*)&P->params[i].ll;
        P->bind[i].is_null = 0;
}


void MysqlPreparedStatement_setDouble(T P, int parameterIndex, double x) {
        TEST_INDEX
        P->params[i].d = x;
        P->bind[i].buffer_type = MYSQL_TYPE_DOUBLE;
        P->bind[i].buffer = (char*)&P->params[i].d;
        P->bind[i].is_null = 0;
}


void MysqlPreparedStatement_setBlob(T P, int parameterIndex, const void *x, int size) {
        TEST_INDEX
        P->bind[i].buffer_type = MYSQL_TYPE_BLOB;
        P->bind[i].buffer = (void*)x;
        if (x==NULL) {
                P->params[i].length = 0;
                P->bind[i].is_null = &yes;
        } else {
                P->params[i].length = size;
                P->bind[i].is_null = 0;
        }
        P->bind[i].length = &P->params[i].length;
}


void MysqlPreparedStatement_execute(T P) {
        assert(P);
        if ((P->paramCount > 0) && (P->lastError = mysql_stmt_bind_param(P->stmt, P->bind)))
                        THROW(SQLException, "%s", mysql_stmt_error(P->stmt));
#if MYSQL_VERSION_ID >= 50002
        {
        unsigned long cursor = CURSOR_TYPE_NO_CURSOR;
        mysql_stmt_attr_set(P->stmt, STMT_ATTR_CURSOR_TYPE, &cursor);
        }
#endif
        if ((P->lastError = mysql_stmt_execute(P->stmt))) 
                THROW(SQLException, "%s", mysql_stmt_error(P->stmt));
        if (P->lastError==MYSQL_OK) {
                /* Discard prepared param data in client/server */
                P->lastError = mysql_stmt_reset(P->stmt);
        }
}


ResultSet_T MysqlPreparedStatement_executeQuery(T P) {
        assert(P);
        if ((P->paramCount > 0) && (P->lastError = mysql_stmt_bind_param(P->stmt, P->bind)))
                        THROW(SQLException, "%s", mysql_stmt_error(P->stmt));
#if MYSQL_VERSION_ID >= 50002
        {
                unsigned long cursor = CURSOR_TYPE_READ_ONLY;
                mysql_stmt_attr_set(P->stmt, STMT_ATTR_CURSOR_TYPE, &cursor);
        }
#endif
        if ((P->lastError = mysql_stmt_execute(P->stmt))) 
                THROW(SQLException, "%s", mysql_stmt_error(P->stmt));
        if (P->lastError==MYSQL_OK)
                return ResultSet_new(MysqlResultSet_new(P->stmt, P->maxRows, true), (Rop_T)&mysqlrops);
        THROW(SQLException, "%s", mysql_stmt_error(P->stmt));
        return NULL;
}


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif

