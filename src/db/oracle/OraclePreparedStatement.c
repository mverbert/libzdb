/*
 * Copyright (C) 2010-2013 Volodymyr Tarasenko <tvntsr@yahoo.com>
 *              2010      Sergey Pavlov <sergey.pavlov@gmail.com>
 *              2010      PortaOne Inc.
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
#include "ConnectionDelegate.h"
#include "OracleConnection.h"


/**
 * Implementation of the PreparedStatement/Delegate interface for oracle.
 *
 * @file
 */


/* ----------------------------------------------------------- Definitions */


const struct Pop_T oraclepops = {
        .name           = "oracle",
        .free           = OraclePreparedStatement_free,
        .setString      = OraclePreparedStatement_setString,
        .setInt         = OraclePreparedStatement_setInt,
        .setLLong       = OraclePreparedStatement_setLLong,
        .setDouble      = OraclePreparedStatement_setDouble,
        .setTimestamp   = OraclePreparedStatement_setTimestamp,
        .setBlob        = OraclePreparedStatement_setBlob,
        .execute        = OraclePreparedStatement_execute,
        .executeQuery   = OraclePreparedStatement_executeQuery,
        .rowsChanged    = OraclePreparedStatement_rowsChanged
};
typedef struct param_t {
        union {
                double real;
                long integer;
                const void *blob;
                const char *string;
                OCINumber number;
                OCIDateTime* date;
        } type;
        int length;
        OCIBind* bind;
} *param_t;
#define T PreparedStatementDelegate_T
struct T {
        int         maxRows;
        ub4         paramCount;
        OCISession* usr;
        OCIStmt*    stmt;
        OCIEnv*     env;
        OCIError*   err;
        OCISvcCtx*  svc;
        param_t     params;
        sword       lastError;
        ub4         rowsChanged;
};

extern const struct Rop_T oraclerops;


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T OraclePreparedStatement_new(OCIStmt *stmt, OCIEnv *env, OCISession* usr, OCIError *err, OCISvcCtx *svc, int max_row) {
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
        P->usr  = usr; 
        P->maxRows = max_row;
        P->lastError = OCI_SUCCESS;
        P->rowsChanged = 0;
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
        assert(P);
        int i = checkAndSetParameterIndex(parameterIndex, P->paramCount);
        P->params[i].type.string = x;
        P->params[i].length = x ? (int)strlen(x) : 0;
        P->lastError = OCIBindByPos(P->stmt, &P->params[i].bind, P->err, parameterIndex, (char *)P->params[i].type.string, 
                                    (int)P->params[i].length, SQLT_CHR, 0, 0, 0, 0, 0, OCI_DEFAULT);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
}


void OraclePreparedStatement_setTimestamp(T P, int parameterIndex, time_t time) {
        assert(P);
        struct tm ts = {.tm_isdst = -1};
        ub4   valid;
        int i = checkAndSetParameterIndex(parameterIndex, P->paramCount);

        P->lastError = OCIDescriptorAlloc((dvoid *)P->env, (dvoid **) &(P->params[i].type.date),
                                          (ub4) OCI_DTYPE_TIMESTAMP,
                                          (size_t) 0, (dvoid **) 0);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));

        gmtime_r(&time, &ts);

        OCIDateTimeConstruct(P->usr,
                             P->err,
                             P->params[i].type.date, //OCIDateTime   *datetime,
                             ts.tm_year+1900, ts.tm_mon+1, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec, 0/*fsec*/,
                             (OraText*)0, 0);
        
        if (OCI_SUCCESS != OCIDateTimeCheck(P->usr, P->err, P->params[i].type.date, &valid) || valid != 0)
        {
                THROW(SQLException, "Invalid date/time value");
        }

        P->params[i].length = sizeof(OCIDateTime *);

        P->lastError = OCIBindByPos(P->stmt, &P->params[i].bind, P->err, parameterIndex, &P->params[i].type.date, 
                                    P->params[i].length, SQLT_TIMESTAMP, 0, 0, 0, 0, 0, OCI_DEFAULT);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
}


void OraclePreparedStatement_setInt(T P, int parameterIndex, int x) {
        assert(P);
        int i = checkAndSetParameterIndex(parameterIndex, P->paramCount);
        P->params[i].type.integer = x;
        P->params[i].length = sizeof(x);
        P->lastError = OCIBindByPos(P->stmt, &P->params[i].bind, P->err, parameterIndex, &P->params[i].type.integer,
                                    (int)P->params[i].length, SQLT_INT, 0, 0, 0, 0, 0, OCI_DEFAULT);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
}


void OraclePreparedStatement_setLLong(T P, int parameterIndex, long long x) {
        assert(P);
        int i = checkAndSetParameterIndex(parameterIndex, P->paramCount);
        P->params[i].length = sizeof(P->params[i].type.number);
        P->lastError = OCINumberFromInt(P->err, &x, sizeof(x), OCI_NUMBER_SIGNED, &P->params[i].type.number);
        if (P->lastError != OCI_SUCCESS)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
        P->lastError = OCIBindByPos(P->stmt, &P->params[i].bind, P->err, parameterIndex, &P->params[i].type.number, 
                                    (int)P->params[i].length, SQLT_VNU, 0, 0, 0, 0, 0, OCI_DEFAULT);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
}


void OraclePreparedStatement_setDouble(T P, int parameterIndex, double x) {
        assert(P);
        int i = checkAndSetParameterIndex(parameterIndex, P->paramCount);
        P->params[i].type.real = x;
        P->params[i].length = sizeof(x);
        P->lastError = OCIBindByPos(P->stmt, &P->params[i].bind, P->err, parameterIndex, &P->params[i].type.real, 
                                    (int)P->params[i].length, SQLT_FLT, 0, 0, 0, 0, 0, OCI_DEFAULT);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
}


void OraclePreparedStatement_setBlob(T P, int parameterIndex, const void *x, int size) {
        assert(P);
        int i = checkAndSetParameterIndex(parameterIndex, P->paramCount);
        P->params[i].type.blob = x;
        P->params[i].length = (x) ? size : 0;
        P->lastError = OCIBindByPos(P->stmt, &P->params[i].bind, P->err, parameterIndex, (void *)P->params[i].type.blob, 
                                    (int)P->params[i].length, SQLT_LNG, 0, 0, 0, 0, 0, OCI_DEFAULT);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
}


void OraclePreparedStatement_execute(T P) {
        assert(P);
        P->rowsChanged = 0;
        P->lastError = OCIStmtExecute(P->svc, P->stmt, P->err, 1, 0, NULL, NULL, OCI_DEFAULT);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
        P->lastError = OCIAttrGet( P->stmt, OCI_HTYPE_STMT, &P->rowsChanged, 0, OCI_ATTR_ROW_COUNT, P->err);
        if (P->lastError != OCI_SUCCESS && P->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
}


ResultSet_T OraclePreparedStatement_executeQuery(T P) {
        assert(P);
        P->rowsChanged = 0;
        P->lastError = OCIStmtExecute(P->svc, P->stmt, P->err, 0, 0, NULL, NULL, OCI_DEFAULT);
        if (P->lastError == OCI_SUCCESS || P->lastError == OCI_SUCCESS_WITH_INFO)
                return ResultSet_new(OracleResultSet_new(P->stmt, P->env, P->usr, P->err, P->svc, false, P->maxRows), (Rop_T)&oraclerops);
        THROW(SQLException, "%s", OraclePreparedStatement_getLastError(P->lastError, P->err));
        return NULL;
}


long long OraclePreparedStatement_rowsChanged(T P) {
        assert(P);
        return P->rowsChanged;
}


/* Error handling: Oracle requires a buffer to store
   error message, to keep error handling thread safe
   TSD is used
*/

/* Key for the thread-specific buffer */
static pthread_key_t error_msg_key;

/* Once-only initialisation of the key */
static pthread_once_t error_msg_key_once = PTHREAD_ONCE_INIT;


/* Return the thread-specific buffer */
static char * get_err_buffer(void) {
        return (char *) pthread_getspecific(error_msg_key);
}

/* Allocate the key */
static void error_msg_key_alloc() {
        pthread_key_create(&error_msg_key, free);
        pthread_setspecific(error_msg_key, malloc(STRLEN));
}

/* This is a general error function also used in OracleResultSet */
const char *OraclePreparedStatement_getLastError(int err, OCIError *errhp) {
        sb4 errcode;
        char* erb;
        pthread_once(&error_msg_key_once, error_msg_key_alloc);
        erb = get_err_buffer();
        assert(erb);
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
