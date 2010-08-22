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
 * Implementation of the ResulSet/Delegate interface for oracle. 
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
typedef struct column_t {
        OCIDefine *def;
        int isNull;
        char *buffer;
        unsigned long length;
} *column_t;
#define T ResultSetImpl_T
struct T {
        int         columnCount;
        int         row;
        ub4         maxRow;
        OCIStmt*    stmt;
        OCIEnv*     env;
        OCIError*   err;
        OCISvcCtx*  svc;
        column_t    columns;
        sword       lastError;
        int         freeStatement;
};

#define TEST_INDEX \
        int i; assert(R);i = columnIndex-1; if (R->columnCount <= 0 || \
        i < 0 || i >= R->columnCount) { THROW(SQLException, "Column index is out of range"); \
        return NULL; } if (R->columns[i].isNull) return NULL;


/* ------------------------------------------------------- Private methods */


static int initaleDefiningBuffers(T R) {
        int i;
        ub2 dtype = 0;
        ub2 fldtype;
        int deptlen;
        int sizelen = sizeof(deptlen);
        OCIParam* pard = NULL;
        for (i = 1; i <= R->columnCount; i++) {
                 deptlen = 0;
                /* The next two statements describe the select-list item, dname, and
                 return its length */
                R->lastError = OCIParamGet(R->stmt, OCI_HTYPE_STMT, R->err, (void **)&pard, i);
                if (R->lastError != OCI_SUCCESS) 
                        return false;
                R->lastError = OCIAttrGet(pard, OCI_DTYPE_PARAM, &deptlen, &sizelen, OCI_ATTR_DATA_SIZE, R->err);
                if (R->lastError != OCI_SUCCESS) { 
                        // cannot get column's size, cleaning and returning
                        OCIDescriptorFree(pard, OCI_DTYPE_PARAM);
                        return false;
                }
                OCIAttrGet(pard, OCI_DTYPE_PARAM, &dtype, 0, OCI_ATTR_DATA_TYPE, R->err); 
                /* Use the retrieved length of dname to allocate an output buffer, and
                 then define the output variable. */
                deptlen +=1;
                R->columns[i-1].length = deptlen;
                switch(dtype) 
                {
                        case SQLT_BLOB: fldtype = SQLT_BIN;  break;
                        case SQLT_CLOB: fldtype = SQLT_CHR; break;
                        default: fldtype = SQLT_CHR; //SQLT_VCS;
                }
                R->columns[i-1].buffer = ALLOC(deptlen + 1);
                R->columns[i-1].isNull = 0;
                R->lastError = OCIDefineByPos(R->stmt, &R->columns[i-1].def, R->err, i, R->columns[i-1].buffer, deptlen, fldtype, &(R->columns[i-1].isNull), 0, 0, OCI_DEFAULT);
                OCIDescriptorFree(pard, OCI_DTYPE_PARAM);
                if (R->lastError != OCI_SUCCESS) {
                        return false;
                }
        }

        return true;
}


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T OracleResultSet_new(OCIStmt *stmt, OCIEnv *env, OCIError *err, OCISvcCtx *svc, int need_free, int max_row) {
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
        R->row = 0;
        R->lastError = OCIAttrGet(R->stmt, OCI_HTYPE_STMT, &R->maxRow, NULL, OCI_ATTR_ROW_COUNT/*OCI_ATTR_ROWS_FETCHED*/, R->err);
        if (R->lastError != OCI_SUCCESS && R->lastError != OCI_SUCCESS_WITH_INFO)
                DEBUG("OracleResultSet_new: Error %d, '%s'\n", R->lastError, OraclePreparedStatement_getLastError(R->lastError,R->err));
        /* Get the number of columns in the select list */
        R->lastError = OCIAttrGet (R->stmt, OCI_HTYPE_STMT, &R->columnCount, NULL, OCI_ATTR_PARAM_COUNT, R->err);
        if (R->lastError != OCI_SUCCESS && R->lastError != OCI_SUCCESS_WITH_INFO)
                DEBUG("OracleResultSet_new: Error %d, '%s'\n", R->lastError, OraclePreparedStatement_getLastError(R->lastError,R->err));
        R->columns = CALLOC(R->columnCount, sizeof (struct column_t));
        if (!initaleDefiningBuffers(R)) {
                DEBUG("OracleResultSet_new: Error %d, '%s'\n", R->lastError, OraclePreparedStatement_getLastError(R->lastError,R->err));
                R->row = -1;
        }
        if ((max_row != 0) && ((R->maxRow > max_row) ||(R->maxRow == 0))) 
                R->maxRow = max_row;
        return R;
}


void OracleResultSet_free(T *R) {
        int i;
        assert(R && *R);
        if ((*R)->freeStatement)
                OCIHandleFree((*R)->stmt, OCI_HTYPE_STMT);
        for (i = 0; i < (*R)->columnCount; i++)
                FREE((*R)->columns[i].buffer);
        free((*R)->columns);
        FREE(*R);
}


int OracleResultSet_getColumnCount(T R) {
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


int OracleResultSet_next(T R) {
        assert(R);
        if ((R->row < 0) || ((R->maxRow > 0) && (R->row >= R->maxRow)))
                return false;
        R->lastError = OCIStmtFetch2(R->stmt, R->err, 1, OCI_FETCH_NEXT, 0, OCI_DEFAULT);
        if (R->lastError == OCI_NO_DATA) 
                return false;
        if (R->lastError != OCI_SUCCESS && R->lastError != OCI_SUCCESS_WITH_INFO)
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(R->lastError, R->err));
        R->row++;
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
        if (status != OCI_SUCCESS) {
                OCIDescriptorFree(pard, OCI_DTYPE_PARAM);
                return -1;
        }
        status = (char_semantics) ?
        /* Retrieve the column width in characters */
        OCIAttrGet(pard, OCI_DTYPE_PARAM, &col_width, NULL, OCI_ATTR_CHAR_SIZE, R->err) :
        /* Retrieve the column width in bytes */
        OCIAttrGet(pard, OCI_DTYPE_PARAM, &col_width, NULL, OCI_ATTR_DATA_SIZE, R->err);
        return (status != OCI_SUCCESS) ? -1 : col_width;
}


const char *OracleResultSet_getString(T R, int columnIndex) {
        TEST_INDEX
        // FIXME Need a way to set R->columns[i].length to the actual length of the current buffer
        R->columns[i].buffer[R->columns[i].length] = 0;
        return R->columns[i].buffer;
}


const void *OracleResultSet_getBlob(T R, int columnIndex, int *size) {
        OCIParam* pard = NULL;
        int sizelen = sizeof(*size);
        TEST_INDEX
        OCIParamGet(R->stmt, OCI_HTYPE_STMT, R->err, (void **)&pard, columnIndex);
        OCIAttrGet(pard, OCI_DTYPE_PARAM, size, &sizelen, OCI_ATTR_DATA_SIZE, R->err);
        OCIDescriptorFree(pard, OCI_DTYPE_PARAM);
        return R->columns[i].buffer;
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif
