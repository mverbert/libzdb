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
 * Implementation of the ResulSet/Strategy interface for oracle. 
 * 
 * @file
 */


/* ----------------------------------------------------------- Definitions */


const struct Rop_T oraclerops = {
        "oracle",
        OracleResultSet_free,
        OracleResultSet_getColumnCount,
        OracleResultSet_getColumnName,
        OracleResultSet_next,
        OracleResultSet_getColumnSize,
        OracleResultSet_getString,
        OracleResultSet_getBlob,
};

#define T ResultSetImpl_T
struct T {
        int         columnCount;
        ub4         maxRow;
        OCIStmt*    stmt;
        OCIEnv*     env;
        OCIError*   err;
        OCISvcCtx*  svc;
        OCIDefine** defnpp;
        void**      dept;
        sword       lastError;
        int         freeStatement;
};


/* ------------------------------------------------------- Private methods */


static void initaleDefiningBuffers(T R) {
        int i;
        ub2 dtype = 0;
        ub2 fldtype;
        int deptlen = 0;
        int sizelen = sizeof(deptlen);
        OCIParam* pard = NULL;
        assert(R);
        for (i = 1; i <= R->columnCount; i++) {
                /* The next two statements describe the select-list item, dname, and
                 return its length */
                OCIParamGet(R->stmt, OCI_HTYPE_STMT, R->err, (void **)&pard, i);
                OCIAttrGet(pard, OCI_DTYPE_PARAM, &deptlen, &sizelen, OCI_ATTR_DATA_SIZE, R->err);
                OCIAttrGet(pard, OCI_DTYPE_PARAM, &dtype, 0, OCI_ATTR_DATA_TYPE, R->err);
                if (R->defnpp[i-1])
                        FREE(R->defnpp[i-1]);
                /* Use the retrieved length of dname to allocate an output buffer, and
                 then define the output variable. 
                 */
                R->dept[i-1] = (text *) ALLOC((int) deptlen + 1);
                fldtype = SQLT_BIN == dtype ? SQLT_LNG : SQLT_STR;
                R->lastError = OCIDefineByPos(R->stmt, &R->defnpp[i-1], R->err, i, R->dept[i-1], (deptlen + 1), fldtype, 0, 0, 0, OCI_DEFAULT);
        }
}


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T OracleResultSet_new(OCIStmt *stmt, OCIEnv *env, OCIError *err, OCISvcCtx *svc, int need_free) {
        T R;
        assert(stmt);
        assert(env);
        assert(err);
        assert(svc);
        NEW(R);
        R->stmt = stmt;
        R->env  = env;
        R->err  = err;
        R->svc  = svc;
        R->freeStatement = need_free;
        R->lastError = OCIAttrGet((void *) R->stmt, OCI_HTYPE_STMT, &R->maxRow, NULL, OCI_ATTR_ROWS_FETCHED, R->err);
        if (R->lastError != OCI_SUCCESS && R->lastError != OCI_SUCCESS_WITH_INFO)
                DEBUG("OracleResultSet_new: Error %d, '%s'\n", R->lastError, OraclePreparedStatement_getLastError(R->lastError,R->err));
        /* get the number of columns in the select list */
        R->lastError = OCIAttrGet (R->stmt, OCI_HTYPE_STMT, &R->columnCount, NULL, OCI_ATTR_PARAM_COUNT, R->err);
        if (R->lastError != OCI_SUCCESS && R->lastError != OCI_SUCCESS_WITH_INFO)
                DEBUG("OracleResultSet_new: Error %d, '%s'\n", R->lastError, OraclePreparedStatement_getLastError(R->lastError,R->err));
        R->defnpp = CALLOC(R->columnCount, sizeof(OCIDefine*));
        R->dept = CALLOC(R->columnCount, sizeof(void*));
        initaleDefiningBuffers(R);
        return R;
}


void OracleResultSet_free(T *R) {
        assert(R && *R);
        if ((*R)->freeStatement)
                OCIHandleFree((*R)->stmt, OCI_HTYPE_STMT);
        free((*R)->defnpp);
        free((*R)->dept);
        FREE(*R);
}


int  OracleResultSet_getColumnCount(T R) {
        assert(R);
        return R->columnCount;
}


const char *OracleResultSet_getColumnName(T R, int column) {
        OCIParam* pard = NULL;
        int col_name_len = 0;
        char* col_name = NULL; 
        sb4 status;
        assert(R);
        if (R->columnCount < column)
                return NULL;
        status = OCIParamGet(R->stmt, OCI_HTYPE_STMT, R->err, (void **)&pard, column);
        if (status != OCI_SUCCESS)
                return NULL;
        status = OCIAttrGet(pard, OCI_DTYPE_PARAM, &col_name, &col_name_len, OCI_ATTR_NAME, R->err);
        return (status != OCI_SUCCESS) ? NULL : col_name;
}


int  OracleResultSet_next(T R) {
        assert(R);
        R->lastError = OCIStmtFetch2(R->stmt, R->err, 1, OCI_FETCH_NEXT, 0, OCI_DEFAULT);
        if (R->lastError == OCI_NO_DATA)
                return false;
        if (R->lastError != OCI_SUCCESS && R->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(R->lastError, R->err));
        return (R->lastError == OCI_SUCCESS);
}


long OracleResultSet_getColumnSize(T R, int columnIndex) {
        OCIParam* pard = NULL;
        ub4 char_semantics = 0;
        sb4 status;
        ub2 col_width = 0;
        assert(R);
        status = OCIParamGet(R->stmt, OCI_HTYPE_STMT, R->err, (void **)&pard, columnIndex);
        if (status != OCI_SUCCESS)
                return -1;
        status = OCIAttrGet(pard, OCI_DTYPE_PARAM, &char_semantics, NULL, OCI_ATTR_CHAR_USED, R->err);
        if (status != OCI_SUCCESS)
                return -1;
        status = (char_semantics) ?
        /* Retrieve the column width in characters */
        OCIAttrGet(pard, OCI_DTYPE_PARAM, &col_width, NULL, OCI_ATTR_CHAR_SIZE, R->err) :
        /* Retrieve the column width in bytes */
        OCIAttrGet(pard, OCI_DTYPE_PARAM, &col_width, NULL, OCI_ATTR_DATA_SIZE, R->err);
        return (status != OCI_SUCCESS) ? -1 : col_width;
}


const char *OracleResultSet_getString(T R, int columnIndex) {
        assert(R);
        if (columnIndex <= 0 || columnIndex > R->columnCount)
                THROW(SQLException, "Column index is out of range");
        return R->dept[columnIndex-1];
}


const void *OracleResultSet_getBlob(T R, int columnIndex, int *size) {
        OCIParam* pard = NULL;
        int sizelen = sizeof(*size);
        assert(R);
        if (columnIndex <= 0 || columnIndex > R->columnCount)
                THROW(SQLException, "Column index is out of range"); 
        OCIParamGet(R->stmt, OCI_HTYPE_STMT, R->err, (void **)&pard, columnIndex);
        OCIAttrGet(pard, OCI_DTYPE_PARAM, size, &sizelen, OCI_ATTR_DATA_SIZE, R->err);
        OCIDescriptorFree(pard, OCI_DTYPE_PARAM);
        return R->dept[columnIndex-1];
}


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif
