/*
 * Copyright (C) 2010 Volodymyr Tarasenko <tvntsr@yahoo.com>
 *               2010 Sergey Pavlov <sergey.pavlov@gmail.com>
 *               2010 PortaOne Inc.
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
#include <stdlib.h>
#include <string.h>

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
 * Implementation of the Connection/Delegate interface for oracle. 
 * 
 * TODO: Query Timeout has no effect as this is not implemented. 
 *
 * @file
 */


/* ----------------------------------------------------------- Definitions */

const struct Cop_T oraclesqlcops = {
        "oracle",
        OracleConnection_onstop,
        OracleConnection_new,
        OracleConnection_free,
        OracleConnection_setQueryTimeout,
        OracleConnection_setMaxRows,
        OracleConnection_ping,
        OracleConnection_beginTransaction,
        OracleConnection_commit,
        OracleConnection_rollback,
        OracleConnection_lastRowId,
        OracleConnection_rowsChanged,
        OracleConnection_execute,
        OracleConnection_executeQuery,
        OracleConnection_prepareStatement,
        OracleConnection_getLastError
};

#define ERB_SIZE 152
#define ORACLE_TRANSACTION_PERIOD 10

#define T ConnectionDelegate_T
struct T {
        URL_T          url;
        OCIEnv*        env;
        OCIError*      err;
        OCISvcCtx*     svc;
        OCISession*    usr;
        OCIServer*     srv;
        OCITrans*      txnhp;
        char           erb[ERB_SIZE];
        int            maxRows;
        int            timeout;
        sword          lastError;
        ub4            rowsChanged;
        StringBuffer_T sb;
};

extern const struct Rop_T oraclerops;
extern const struct Pop_T oraclepops;


/* ------------------------------------------------------- Private methods */


static int doConnect(T C, URL_T url, char**  error) {
#define ERROR(e) do {*error = Str_dup(e); return false;} while (0)
#define ORAERROR(e) do{ *error = Str_dup(OracleConnection_getLastError(e)); return false;} while(0)
        const char *database, *username, *password;
        const char *host = URL_getHost(url);
        int port = URL_getPort(url);
        if (! (username = URL_getUser(url)))
                if (! (username = URL_getParameter(url, "user")))
                        ERROR("no username specified in URL");
        if (! (password = URL_getPassword(url)))
                if (! (password = URL_getParameter(url, "password")))
                        ERROR("no password specified in URL");
        if (! (database = URL_getPath(url)))
                ERROR("no database specified in URL");
        ++database;
        /* Create a thread-safe OCI environment with N' substitution turned on. */
        if (OCIEnvCreate(&C->env, OCI_THREADED | OCI_NCHAR_LITERAL_REPLACE_ON, 0, 0, 0, 0, 0, 0))
                ERROR("Create a OCI environment failed");
        /* allocate an error handle */
        if (OCI_SUCCESS != OCIHandleAlloc(C->env, (dvoid**)&C->err, OCI_HTYPE_ERROR, 0, 0))
                ERROR("Allocating error handler failed");
        /* server contexts */
        if (OCI_SUCCESS != OCIHandleAlloc(C->env, (dvoid**)&C->srv, OCI_HTYPE_SERVER, 0, 0))
                ERROR("Allocating server context failed");
        /* allocate a service handle */
        if (OCI_SUCCESS != OCIHandleAlloc(C->env, (dvoid**)&C->svc, OCI_HTYPE_SVCCTX, 0, 0))
                ERROR("Allocating service handle failed");
        StringBuffer_clear(C->sb);
        /* Oracle connect string is on the form: //host[:port]/service name */
        if (host) {
                StringBuffer_append(C->sb, "//%s", host);
                if (port > 0)
                        StringBuffer_append(C->sb, ":%d", port);
                StringBuffer_append(C->sb, "/%s", database);
        } else /* Or just service name */
                StringBuffer_append(C->sb, "%s", database);
        /* Create a server context */
        C->lastError = OCIServerAttach(C->srv, C->err, StringBuffer_toString(C->sb), StringBuffer_length(C->sb), 0);
        if (C->lastError != OCI_SUCCESS && C->lastError != OCI_SUCCESS_WITH_INFO)
                ORAERROR(C);
        /* Set attribute server context in the service context */
        C->lastError = OCIAttrSet(C->svc, OCI_HTYPE_SVCCTX, C->srv, 0, OCI_ATTR_SERVER, C->err);
        if (C->lastError != OCI_SUCCESS && C->lastError != OCI_SUCCESS_WITH_INFO)
                ORAERROR(C);
        C->lastError = OCIHandleAlloc(C->env, (void**)&C->usr, OCI_HTYPE_SESSION, 0, NULL);
        if (C->lastError != OCI_SUCCESS && C->lastError != OCI_SUCCESS_WITH_INFO)
                ORAERROR(C);
        C->lastError = OCIAttrSet(C->usr, OCI_HTYPE_SESSION, (dvoid *)username, (int)strlen(username), OCI_ATTR_USERNAME, C->err);
        if (C->lastError != OCI_SUCCESS && C->lastError != OCI_SUCCESS_WITH_INFO)
                ORAERROR(C);
        C->lastError = OCIAttrSet(C->usr, OCI_HTYPE_SESSION, (dvoid *)password, (int)strlen(password), OCI_ATTR_PASSWORD, C->err);
        if (C->lastError != OCI_SUCCESS && C->lastError != OCI_SUCCESS_WITH_INFO)
                ORAERROR(C);
        C->lastError = OCISessionBegin(C->svc, C->err, C->usr, OCI_CRED_RDBMS, OCI_DEFAULT);
        if (C->lastError != OCI_SUCCESS && C->lastError != OCI_SUCCESS_WITH_INFO)
                ORAERROR(C);
        OCIAttrSet(C->svc, OCI_HTYPE_SVCCTX, C->usr, 0, OCI_ATTR_SESSION, C->err);
        return true;
}


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T OracleConnection_new(URL_T url, char **error) {
        T C;
        assert(url);
        assert(error);
        NEW(C);
        C->url = url;
        C->sb = StringBuffer_create(STRLEN);
        C->timeout = SQL_DEFAULT_TIMEOUT;
        if (! doConnect(C, url, error)) {
                OracleConnection_free(&C);
                return NULL;
        }
        C->txnhp = NULL;
        return C;
}


void OracleConnection_free(T* C) {
        assert(C && *C);
        if ((*C)->svc)
                OCISessionEnd((*C)->svc, (*C)->err, (*C)->usr, OCI_DEFAULT);
        if ((*C)->srv)
                OCIServerDetach((*C)->srv, (*C)->err, OCI_DEFAULT);
        if ((*C)->env)
                OCIHandleFree((*C)->env, OCI_HTYPE_ENV);
        StringBuffer_free(&(*C)->sb);
        FREE(*C);
}


void OracleConnection_setQueryTimeout(T C, int ms) {
        assert(C);
        C->timeout = ms;
}


void OracleConnection_setMaxRows(T C, int max) {
        assert(C);
        C->maxRows = max;
}


int  OracleConnection_ping(T C) {
        ub4 serverStatus = 0;
        C->lastError = OCIAttrGet(C->srv, OCI_HTYPE_SERVER, &serverStatus, NULL, OCI_ATTR_SERVER_STATUS, C->err);
        return serverStatus == OCI_SERVER_NORMAL;
}


int  OracleConnection_beginTransaction(T C) {
        assert(C);
        if (C->txnhp == NULL) /* Allocate handler only once, if it is necessary */
        {
            /* allocate transaction handle and set it in the service handle */
            C->lastError = OCIHandleAlloc(C->env, (void **)&C->txnhp, OCI_HTYPE_TRANS, 0, 0);
            if (C->lastError != OCI_SUCCESS) 
                return false;
            OCIAttrSet(C->svc, OCI_HTYPE_SVCCTX, (void *)C->txnhp, 0, OCI_ATTR_TRANS, C->err);
        }
        C->lastError = OCITransStart (C->svc, C->err, ORACLE_TRANSACTION_PERIOD, OCI_TRANS_NEW);
        return (C->lastError == OCI_SUCCESS);
}


int  OracleConnection_commit(T C) {
        assert(C);
        C->lastError = OCITransCommit(C->svc, C->err, OCI_DEFAULT);
        return C->lastError == OCI_SUCCESS;
}


int  OracleConnection_rollback(T C) {
        assert(C);
        C->lastError = OCITransRollback(C->svc, C->err, OCI_DEFAULT);
        return C->lastError == OCI_SUCCESS;
}


long long int OracleConnection_lastRowId(T C) {
        /*:FIXME:*/
        /*
         Oracle's RowID can be mapped on string only
         so, currently I leave it unimplemented
         */
        
        /*     OCIRowid* rowid; */
        /*     OCIDescriptorAlloc((dvoid *)C->env,  */
        /*                        (dvoid **)&rowid, */
        /*                        (ub4) OCI_DTYPE_ROWID,  */
        /*                        (size_t) 0, (dvoid **) 0); */
        
        /*     if (OCIAttrGet (select_p, */
        /*                     OCI_HTYPE_STMT, */
        /*                     &rowid,              /\* get the current rowid *\/ */
        /*                     0, */
        /*                     OCI_ATTR_ROWID, */
        /*                     errhp)) */
        /*     { */
        /*         printf ("Getting the Rowid failed \n"); */
        /*         return (OCI_ERROR); */
        /*     } */
        
        /*     OCIDescriptorFree(rowid, OCI_DTYPE_ROWID); */
        DEBUG("OracleConnection_lastRowId: Not implemented yet");
        return -1;
}


long long int OracleConnection_rowsChanged(T C) {
        assert(C);
        return C->rowsChanged;
}


int  OracleConnection_execute(T C, const char *sql, va_list ap) {
        OCIStmt* stmtp;
        va_list ap_copy;
        assert(C);
        C->rowsChanged = 0;
        StringBuffer_clear(C->sb);
        va_copy(ap_copy, ap);
        StringBuffer_vappend(C->sb, sql, ap_copy);
        va_end(ap_copy);
        StringBuffer_trim(C->sb);
        /* Build statement */
        C->lastError = OCIHandleAlloc(C->env, (void **)&stmtp, OCI_HTYPE_STMT, 0, NULL);
        if (C->lastError != OCI_SUCCESS && C->lastError != OCI_SUCCESS_WITH_INFO)
                return false;
        C->lastError = OCIStmtPrepare(stmtp, C->err, StringBuffer_toString(C->sb), StringBuffer_length(C->sb), OCI_NTV_SYNTAX, OCI_DEFAULT);
        if (C->lastError != OCI_SUCCESS && C->lastError != OCI_SUCCESS_WITH_INFO) {
                OCIHandleFree(stmtp, OCI_HTYPE_STMT);
                return false;
        }
        /* Execute */
        C->lastError = OCIStmtExecute(C->svc, stmtp, C->err, 1, 0, NULL, NULL, OCI_DEFAULT);
        if (C->lastError != OCI_SUCCESS && C->lastError != OCI_SUCCESS_WITH_INFO) {
                ub4 parmcnt = 0;
                OCIAttrGet(stmtp, OCI_HTYPE_STMT, &parmcnt, NULL, OCI_ATTR_PARSE_ERROR_OFFSET, C->err);
                DEBUG("Error occured in StmtExecute %d (%s), offset is %d\n", C->lastError, OracleConnection_getLastError(C), parmcnt);
                OCIHandleFree(stmtp, OCI_HTYPE_STMT);
                return false;
        }
        C->lastError = OCIAttrGet(stmtp, OCI_HTYPE_STMT, &C->rowsChanged, 0, OCI_ATTR_ROW_COUNT, C->err);
        if (C->lastError != OCI_SUCCESS && C->lastError != OCI_SUCCESS_WITH_INFO)
                DEBUG("OracleConnection_execute: Error in OCIAttrGet %d (%s)\n", C->lastError, OracleConnection_getLastError(C));
        OCIHandleFree(stmtp, OCI_HTYPE_STMT);
        return C->lastError == OCI_SUCCESS;
}


ResultSet_T OracleConnection_executeQuery(T C, const char *sql, va_list ap) {
        OCIStmt* stmtp;
        va_list  ap_copy;
        assert(C);
        C->rowsChanged = 0;
        StringBuffer_clear(C->sb);
        va_copy(ap_copy, ap);
        StringBuffer_vappend(C->sb, sql, ap_copy);
        va_end(ap_copy);
        StringBuffer_trim(C->sb);
        /* Build statement */
        C->lastError = OCIHandleAlloc(C->env, (void **)&stmtp, OCI_HTYPE_STMT, 0, NULL);
        if (C->lastError != OCI_SUCCESS && C->lastError != OCI_SUCCESS_WITH_INFO)
                return NULL;
        C->lastError = OCIStmtPrepare(stmtp, C->err, StringBuffer_toString(C->sb), StringBuffer_length(C->sb), OCI_NTV_SYNTAX, OCI_DEFAULT);
        if (C->lastError != OCI_SUCCESS && C->lastError != OCI_SUCCESS_WITH_INFO) {
                OCIHandleFree(stmtp, OCI_HTYPE_STMT);
                return NULL;
        }
        /* Execute and create Result Set */
        C->lastError = OCIStmtExecute(C->svc, stmtp, C->err, 0, 0, NULL, NULL, OCI_DEFAULT);    
        if (C->lastError != OCI_SUCCESS && C->lastError != OCI_SUCCESS_WITH_INFO) {
                ub4 parmcnt = 0;
                OCIAttrGet(stmtp, OCI_HTYPE_STMT, &parmcnt, NULL, OCI_ATTR_PARSE_ERROR_OFFSET, C->err);
                DEBUG("Error occured in StmtExecute %d (%s), offset is %d\n", C->lastError, OracleConnection_getLastError(C), parmcnt);
                OCIHandleFree(stmtp, OCI_HTYPE_STMT);
                return NULL;
        }
        C->lastError = OCIAttrGet(stmtp, OCI_HTYPE_STMT, &C->rowsChanged, 0, OCI_ATTR_ROW_COUNT, C->err);
        if (C->lastError != OCI_SUCCESS && C->lastError != OCI_SUCCESS_WITH_INFO)
                DEBUG("OracleConnection_execute: Error in OCIAttrGet %d (%s)\n", C->lastError, OracleConnection_getLastError(C));
        return ResultSet_new(OracleResultSet_new(stmtp, C->env, C->err, C->svc, true, C->maxRows), (Rop_T)&oraclerops);
}


PreparedStatement_T OracleConnection_prepareStatement(T C, const char *sql, va_list ap) {
        OCIStmt *stmtp;
        va_list ap_copy;
        assert(C);
        StringBuffer_clear(C->sb);
        va_copy(ap_copy, ap);
        StringBuffer_vappend(C->sb, sql, ap_copy);
        va_end(ap_copy);
        StringBuffer_trim(C->sb);
        StringBuffer_prepare4oracle(C->sb);
        /* Build statement */
        C->lastError = OCIHandleAlloc(C->env, (void **)&stmtp, OCI_HTYPE_STMT, 0, 0);
        if (C->lastError != OCI_SUCCESS && C->lastError != OCI_SUCCESS_WITH_INFO)
                return NULL;
        C->lastError = OCIStmtPrepare(stmtp, C->err, StringBuffer_toString(C->sb), StringBuffer_length(C->sb), OCI_NTV_SYNTAX, OCI_DEFAULT);
        if (C->lastError != OCI_SUCCESS && C->lastError != OCI_SUCCESS_WITH_INFO) {
                OCIHandleFree(stmtp, OCI_HTYPE_STMT);
                return NULL;
        }
        return PreparedStatement_new(OraclePreparedStatement_new(stmtp, C->env, C->err, C->svc, C->maxRows), (Pop_T)&oraclepops);
}


const char *OracleConnection_getLastError(T C) {
        sb4 errcode;
        switch (C->lastError)
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
                        (void) OCIErrorGet(C->err, 1, NULL, &errcode, C->erb, (ub4)ERB_SIZE, OCI_HTYPE_ERROR);
                        return C->erb;
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
        return C->erb;
}

/* Oracle client library finalization */
void OracleConnection_onstop(void) {
        // Not needed, OracleConnection_free below handle finalization
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif
