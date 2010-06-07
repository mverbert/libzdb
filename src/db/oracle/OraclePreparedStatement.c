/*
 * Copyright (C) 2010 Volodymyr Tarasenko <tvntsr@yahoo.com>
 *               2010 Sergey Pavlov <sergey.pavlov@gmail.com>
 *               2010 PortaOne Inc.
 * Copyright (C) 2010 Tildeslash Ltd. All rights reserved.
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
#include <math.h>

#include <oci.h>

#include "URL.h"
#include "ResultSet.h"
#include "StringBuffer.h"
#include "PreparedStatement.h"
#include "OracleResultSet.h"
#include "OraclePreparedStatement.h"
#include "ConnectionStrategy.h"
#include "OracleConnection.h"


/**
 * Implementation of the PreparedStatement/Strategy interface for oracle.
 *
 * @file
 */


/* ----------------------------------------------------------- Definitions */


const struct Pop_T oraclepops = {
        "oracle",
        OraclePreparedStatement_free,
        OraclePreparedStatement_setString,
        OraclePreparedStatement_setInt,
        OraclePreparedStatement_setLLong,
        OraclePreparedStatement_setDouble,
        OraclePreparedStatement_setBlob,
        OraclePreparedStatement_execute,
        OraclePreparedStatement_executeQuery
};
typedef struct param_t {
        union {
                double real;
                long integer;
                const void *blob;
                const char *string;
                long long int llong;
        };
        long length;
        OCIBind* bind;
} *param_t;
#define T PreparedStatementImpl_T
struct T {
        int        maxRows;
        ub4        paramCount;
        OCIStmt*   stmt;
        OCIEnv*    env;
        OCIError*  err;
        OCISvcCtx* svc;
        param_t    params;
        sword      lastError;
};

#define TEST_INDEX \
        int i; assert(P); i = parameterIndex - 1; if (P->paramCount <= 0 || \
        i < 0 || i >= P->paramCount) THROW(SQLException, "Parameter index is out of range"); 

extern const struct Rop_T oraclerops;


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T OraclePreparedStatement_new(OCIStmt *stmt, OCIEnv *env, OCIError *err, OCISvcCtx *svc, int max_row) {
        T P;
        assert(stmt);
        assert(env);
        assert(err);
        assert(svc);
        NEW(P);
        P->stmt = stmt;
        P->env  = env;
        P->err  = err;
        P->svc  = svc;
        P->maxRows = max_row;
        P->lastError = OCI_SUCCESS;
        /* paramCount */
        P->lastError = OCIAttrGet(P->stmt, OCI_HTYPE_STMT, &P->paramCount, NULL, OCI_ATTR_BIND_COUNT, P->err);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                P->paramCount = 0; 
        if (P->paramCount)
                P->params = CALLOC(P->paramCount, sizeof(struct param_t));
        return P;
}


void OraclePreparedStatement_free(T *P) {
        assert(P && *P);
        OCIHandleFree((*P)->stmt, OCI_HTYPE_STMT);
        if ((*P)->params) {
                // (*P)->params[i].bind is freed implicitly when the statement handle is deallocated
                FREE((*P)->params);
        }
        FREE(*P);
}


void OraclePreparedStatement_setString(T P, int parameterIndex, const char *x) {
        TEST_INDEX
        P->params[i].string = x;
        P->params[i].length = x ? strlen(x) : 0;
        P->lastError = OCIBindByPos(P->stmt, &P->params[i].bind, P->err, parameterIndex, (char *)P->params[i].string, 
                                    P->params[i].length, SQLT_CHR, 0, 0, 0, 0, 0, OCI_DEFAULT);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
}


void OraclePreparedStatement_setInt(T P, int parameterIndex, int x) {
        TEST_INDEX
        P->params[i].integer = x;
        P->params[i].length = sizeof(x);
        P->lastError = OCIBindByPos(P->stmt, &P->params[i].bind, P->err, parameterIndex, &P->params[i].integer, 
                                    P->params[i].length, SQLT_INT, 0, 0, 0, 0, 0, OCI_DEFAULT);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
}


void OraclePreparedStatement_setLLong(T P, int parameterIndex, long long int x) {
        TEST_INDEX
        P->params[i].llong = x;
        P->params[i].length = sizeof(x);
        P->lastError = OCIBindByPos(P->stmt, &P->params[i].bind, P->err, parameterIndex, &P->params[i].llong, 
                                    P->params[i].length, SQLT_LVB, 0, 0, 0, 0, 0, OCI_DEFAULT);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
}


void OraclePreparedStatement_setDouble(T P, int parameterIndex, double x) {
        TEST_INDEX
        P->params[i].real = x;
        P->params[i].length = sizeof(x);
        P->lastError = OCIBindByPos(P->stmt, &P->params[i].bind, P->err, parameterIndex, &P->params[i].real, 
                                    P->params[i].length, SQLT_FLT, 0, 0, 0, 0, 0, OCI_DEFAULT);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
}


void OraclePreparedStatement_setBlob(T P, int parameterIndex, const void *x, int size) {
        TEST_INDEX
        P->params[i].blob = x;
        P->params[i].length = (x) ? size : 0;
        P->lastError = OCIBindByPos(P->stmt, &P->params[i].bind, P->err, parameterIndex, (void *)P->params[i].blob, 
                                    P->params[i].length, SQLT_BIN, 0, 0, 0, 0, 0, OCI_DEFAULT);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
}


void OraclePreparedStatement_execute(T P) {
        assert(P);
        P->lastError = OCIStmtExecute(P->svc, P->stmt, P->err, 1, 0, NULL, NULL, OCI_DEFAULT);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
}


ResultSet_T OraclePreparedStatement_executeQuery(T P) {
        assert(P);
        P->lastError = OCIStmtExecute(P->svc, P->stmt, P->err, 0, 0, NULL, NULL, OCI_DEFAULT);
        if (P->lastError == OCI_SUCCESS || P->lastError == OCI_SUCCESS_WITH_INFO)
                return ResultSet_new(OracleResultSet_new(P->stmt, P->env, P->err, P->svc, false, P->maxRows), (Rop_T)&oraclerops);
        THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
        return NULL;
}


/* This is a general error function also used in OracleResultSet */
const char *OraclePreparedStatement_getLastError(int err, OCIError *errhp) {
        sb4 errcode;
        static char erb[STRLEN]; // TODO: This is wrong in a multi-threaded environment
        assert(errhp);
        switch (err)
        {
                case OCI_SUCCESS:
                        return "";
                case OCI_SUCCESS_WITH_INFO:
                        return "Error - OCI_SUCCESS_WITH_INFO";
                        break;
                case OCI_NEED_DATA:
                        return "Error - OCI_NEED_DATA";
                        break;
                case OCI_NO_DATA:
                        return "Error - OCI_NODATA";
                        break;
                case OCI_ERROR:
                        OCIErrorGet(errhp, 1, NULL, &errcode, erb, STRLEN, OCI_HTYPE_ERROR);
                        return erb;
                        break;
                case OCI_INVALID_HANDLE:
                        return "Error - OCI_INVALID_HANDLE";
                        break;
                case OCI_STILL_EXECUTING:
                        return "Error - OCI_STILL_EXECUTE";
                        break;
                case OCI_CONTINUE:
                        return "Error - OCI_CONTINUE";
                        break;
                default:
                        break;
        }
        return erb;
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif
