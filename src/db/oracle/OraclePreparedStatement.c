/*
 * Copyright (C) 2010 Volodymyr Tarasenko <tvntsr@yahoo.com>
 *               2010 Sergey Pavlov <sergey.pavlov@gmail.com>
 *               2010 PortaOne Inc.
 * Copyright (C) 2004-2010 Tildeslash Ltd. All rights reserved.
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

#define T PreparedStatementImpl_T
struct T {
        /* int maxRows; */
        ub4        paramCount;
        OCIStmt*   stmt;
        OCIEnv*    env;
        OCIError*  err;
        OCISvcCtx* svc;
        OCIBind**  bindpp;
        void**     data;
        sword      lastError;
};

#define STRDUP(s) (s ? Str_dup(s) : NULL)

extern const struct Rop_T oraclerops;


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T OraclePreparedStatement_new(const OraText *stmt, ub4 stmt_len, OCIEnv *env, OCIError *errhp, OCISvcCtx *svc, sword *lastError) {
        T P;
        OCIStmt *stmtp;
        assert(stmt);
        *lastError = OCIHandleAlloc(env, (void **)&stmtp, OCI_HTYPE_STMT, 0, 0);
        *lastError = OCIStmtPrepare(stmtp, errhp, stmt, stmt_len, OCI_NTV_SYNTAX, OCI_DEFAULT);
        if (*lastError != OCI_SUCCESS && *lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(*lastError, errhp));
        NEW(P);
        P->stmt = stmtp;
        P->env  = env;
        P->err  = errhp;
        P->svc  = svc;
        P->lastError = OCI_SUCCESS;
        /* paramCount */
        *lastError = OCIAttrGet(stmtp, OCI_HTYPE_STMT, &P->paramCount, NULL, OCI_ATTR_BIND_COUNT, errhp);
        if (*lastError != OCI_SUCCESS && *lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(*lastError, errhp));
        if (P->paramCount) {
                P->bindpp = CALLOC(P->paramCount, sizeof(OCIBind*));
                P->data   = CALLOC(P->paramCount, sizeof(void*));
        }
        return P;
}


void OraclePreparedStatement_free(T *P) {
        assert(P && *P);
        OCIHandleFree((*P)->stmt, OCI_HTYPE_STMT);
        if ((*P)->bindpp) {
                int i;
                for (i = 0; i < (*P)->paramCount; i++) {
                        OCIHandleFree((*P)->bindpp[i], OCI_HTYPE_BIND);
                        (*P)->bindpp[i] = NULL;
                        FREE((*P)->data[i]);
                }
                FREE((*P)->data);
                FREE((*P)->bindpp);
        }
        FREE(*P);
}


void OraclePreparedStatement_setString(T P, int parameterIndex, const char *x) {
        assert(P);
        if ((P->paramCount <= 0) || (parameterIndex > P->paramCount))
                THROW(SQLException, "Parameter index is out of range"); 
        if (P->data[parameterIndex-1])
                FREE(P->data[parameterIndex-1]);
        P->data[parameterIndex-1] = STRDUP(x);
        P->lastError = OCIBindByPos(P->stmt, &P->bindpp[parameterIndex-1], P->err, parameterIndex, 
                                    P->data[parameterIndex-1], strlen(x), SQLT_CHR, 0, 0, 0, 0, 0, OCI_DEFAULT);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
}


void OraclePreparedStatement_setInt(T P, int parameterIndex, int x) {
        assert(P);
        if ((P->paramCount <= 0) || (parameterIndex > P->paramCount))
                THROW(SQLException, "Parameter index is out of range"); 
        if (P->data[parameterIndex-1])
                FREE(P->data[parameterIndex-1]);
        P->data[parameterIndex-1] = ALLOC(sizeof(x));
        *(int*)(P->data[parameterIndex-1]) = x;
        P->lastError = OCIBindByPos(P->stmt, &P->bindpp[parameterIndex - 1], P->err, parameterIndex, P->data[parameterIndex-1], 
                                    sizeof(x), SQLT_INT, 0, 0, 0, 0, 0, OCI_DEFAULT);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
}


void OraclePreparedStatement_setLLong(T P, int parameterIndex, long long int x) {
        size_t s;
        assert(P);
        if ((P->paramCount <= 0) || (parameterIndex > P->paramCount))
                THROW(SQLException, "Parameter index is out of range"); 
        if (P->data[parameterIndex-1])
                FREE(P->data[parameterIndex-1]);
        s = 2+log10(x);
        P->data[parameterIndex-1] = ALLOC(2+log10(x));
        snprintf(P->data[parameterIndex-1], s, "%lld", x);
        P->lastError = OCIBindByPos(P->stmt, &P->bindpp[parameterIndex-1], P->err, parameterIndex, P->data[parameterIndex-1], s,
                                    SQLT_STR, 0, 0, 0, 0, 0, OCI_DEFAULT);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
}


void OraclePreparedStatement_setDouble(T P, int parameterIndex, double x) {
        assert(P);
        if ((P->paramCount <= 0) || (parameterIndex > P->paramCount))
                THROW(SQLException, "Parameter index is out of range"); 
        if (P->data[parameterIndex-1])
                FREE(P->data[parameterIndex-1]);
        P->data[parameterIndex-1] = ALLOC(sizeof(x));
        *(double*)(P->data[parameterIndex-1]) = x;
        P->lastError = OCIBindByPos(P->stmt, &P->bindpp[parameterIndex - 1], P->err, parameterIndex, P->data[parameterIndex-1], 
                                    sizeof(x), SQLT_FLT, 0, 0, 0, 0, 0, OCI_DEFAULT);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
}


void OraclePreparedStatement_setBlob(T P, int parameterIndex, const void *x, int size) {
        assert(P);
        if ((P->paramCount <= 0) || (parameterIndex > P->paramCount))
                THROW(SQLException, "Parameter index is out of range"); 
        if (P->data[parameterIndex-1])
                FREE(P->data[parameterIndex-1]);
        P->data[parameterIndex-1] = NULL;
        P->lastError = OCIBindByPos(P->stmt, &P->bindpp[parameterIndex - 1], P->err, parameterIndex, (void *)x, size,
                                    SQLT_LNG, 0, 0, 0, 0, 0, OCI_DEFAULT);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
}


void OraclePreparedStatement_execute(T P) {
        assert(P);
        /* execute and fetch */
        P->lastError = OCIStmtExecute(P->svc, P->stmt, P->err, 1, 0, NULL, NULL, OCI_DEFAULT);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
}


ResultSet_T OraclePreparedStatement_executeQuery(T P) {
        assert(P);
        /* execute and fetch */
        P->lastError = OCIStmtExecute(P->svc, P->stmt, P->err, 0, 0, NULL, NULL, OCI_DEFAULT);
        if (P->lastError == OCI_SUCCESS || P->lastError == OCI_SUCCESS_WITH_INFO)
                return ResultSet_new(OracleResultSet_new(P->stmt, P->env, P->err, P->svc, 0), (Rop_T)&oraclerops);
        THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
        return NULL;
}


const char *OraclePreparedStatement_getLastError(int err, OCIError*  errhp) {
        sb4 errcode;
        static char erb[1024]; // FIXME this is wrong in a multi-threaded environment
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
                        OCIErrorGet((dvoid *)errhp, 1, NULL, &errcode, erb, 1024, OCI_HTYPE_ERROR);
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
