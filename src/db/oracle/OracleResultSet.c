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
        char *name;
        unsigned long length;
        OCILobLocator *lob_loc;
} *column_t;
#define T ResultSetDelegate_T
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

#ifndef ORACLE_COLUMN_NAME_LOWERCASE
#define ORACLE_COLUMN_NAME_LOWERCASE 2
#endif
#define LOB_CHUNK_SIZE  2000
#define TEST_INDEX \
        int i; assert(R);i = columnIndex-1; if (R->columnCount <= 0 || \
        i < 0 || i >= R->columnCount) { THROW(SQLException, "Column index is out of range"); } 


/* ------------------------------------------------------- Private methods */


static int initaleDefiningBuffers(T R) {
        ub2 dtype = 0;
        int deptlen;
        int sizelen = sizeof(deptlen);
        OCIParam* pard = NULL;
        sword status;
        for (int i = 1; i <= R->columnCount; i++) {
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
                R->columns[i-1].isNull = 0;
                switch(dtype) 
                {
                        case SQLT_BLOB:
                                R->columns[i-1].buffer = NULL;
                                status = OCIDescriptorAlloc((dvoid *)R->env, (dvoid **) &(R->columns[i-1].lob_loc),
                                                (ub4) OCI_DTYPE_LOB,
                                                (size_t) 0, (dvoid **) 0);
                                R->lastError = OCIDefineByPos(R->stmt, &R->columns[i-1].def, R->err, i, 
                                        &(R->columns[i-1].lob_loc), deptlen, SQLT_BLOB, &(R->columns[i-1].isNull), 0, 0, OCI_DEFAULT);
                                break;

                        case SQLT_CLOB: 
                                R->columns[i-1].buffer = NULL;
                                status = OCIDescriptorAlloc((dvoid *)R->env, (dvoid **) &(R->columns[i-1].lob_loc),
                                                (ub4) OCI_DTYPE_LOB,
                                                (size_t) 0, (dvoid **) 0);
                                R->lastError = OCIDefineByPos(R->stmt, &R->columns[i-1].def, R->err, i, 
                                        &(R->columns[i-1].lob_loc), deptlen, SQLT_CLOB, &(R->columns[i-1].isNull), 0, 0, OCI_DEFAULT);
                                break;
                        default:
                                R->columns[i-1].lob_loc = NULL;
                                R->columns[i-1].buffer = ALLOC(deptlen + 1);
                                R->lastError = OCIDefineByPos(R->stmt, &R->columns[i-1].def, R->err, i, 
                                        R->columns[i-1].buffer, deptlen, SQLT_STR, &(R->columns[i-1].isNull), 0, 0, OCI_DEFAULT);
                }
                {
                        char *col_name;
                        ub4   col_name_len;

                        R->lastError = OCIAttrGet(pard, OCI_DTYPE_PARAM, &col_name, &col_name_len, OCI_ATTR_NAME, R->err);
                        if (R->lastError != OCI_SUCCESS)
                                continue;
#if defined(ORACLE_COLUMN_NAME_LOWERCASE) && ORACLE_COLUMN_NAME_LOWERCASE > 1
                        R->columns[i-1].name = CALLOC(1, col_name_len);
                        OCIMultiByteStrCaseConversion(R->env, R->columns[i-1].name, col_name, OCI_NLS_LOWERCASE);
#else
                        R->columns[i-1].name = Str_dup(col_name);
#endif /*COLLUMN_NAME_LOWERCASE*/
                }
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
        assert(R && *R);
        if ((*R)->freeStatement)
                OCIHandleFree((*R)->stmt, OCI_HTYPE_STMT);
        for (int i = 0; i < (*R)->columnCount; i++) {
                if ((*R)->columns[i].lob_loc)
                        OCIDescriptorFree((*R)->columns[i].lob_loc, OCI_DTYPE_LOB);
                FREE((*R)->columns[i].buffer);
                FREE((*R)->columns[i].name);
        }
        FREE((*R)->columns);
        FREE(*R);
}


int OracleResultSet_getColumnCount(T R) {
        assert(R);
        return R->columnCount;
}


const char *OracleResultSet_getColumnName(T R, int column) {
        assert(R);
        if (R->columnCount < column)
                return NULL;
        return  R->columns[column-1].name;
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
	if (R->lastError == OCI_SUCCESS_WITH_INFO)
		DEBUG("OracleResultSet_next Error %d, '%s'\n", R->lastError, OraclePreparedStatement_getLastError(R->lastError, R->err));
        R->row++;
        return ((R->lastError == OCI_SUCCESS) || (R->lastError == OCI_SUCCESS_WITH_INFO));
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
        if (R->columns[i].isNull) 
                return NULL;
        if (R->columns[i].buffer)
                R->columns[i].buffer[R->columns[i].length] = 0;
        return R->columns[i].buffer;
}


const void *OracleResultSet_getBlob(T R, int columnIndex, int *size) {
        TEST_INDEX
        if (R->columns[i].isNull) 
                return NULL;
        if (R->columns[i].buffer)
                FREE(R->columns[i].buffer);
        oraub8 read_chars = 0;
        oraub8 read_bytes = 0;
        oraub8 total_bytes = 0;
        R->columns[i].buffer = ALLOC(LOB_CHUNK_SIZE);
        *size = 0;
        ub1 piece = OCI_FIRST_PIECE;
        do {
                read_bytes = 0;
                read_chars = 0;
                R->lastError = OCILobRead2(R->svc, R->err, R->columns[i].lob_loc, &read_bytes, &read_chars, 1, 
                                R->columns[i].buffer + total_bytes, LOB_CHUNK_SIZE, piece, NULL, NULL, 0, SQLCS_IMPLICIT);
                if (read_bytes) {
                        total_bytes += read_bytes;
                        piece = OCI_NEXT_PIECE;
                        R->columns[i].buffer = RESIZE(R->columns[i].buffer, total_bytes + LOB_CHUNK_SIZE);
                }
        } while (R->lastError == OCI_NEED_DATA);
        if (R->lastError != OCI_SUCCESS && R->lastError != OCI_SUCCESS_WITH_INFO) {
                FREE(R->columns[i].buffer);
                R->columns[i].buffer = NULL;
                THROW(SQLException, "%s", OraclePreparedStatement_getLastError(R->lastError, R->err));
        }
        *size = R->columns[i].length = (int)total_bytes;
        return (const void *)R->columns[i].buffer;
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif
