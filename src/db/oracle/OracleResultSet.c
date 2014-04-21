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
	.name           = "oracle",
        .free           = OracleResultSet_free,
        .getColumnCount = OracleResultSet_getColumnCount,
        .getColumnName  = OracleResultSet_getColumnName,
        .getColumnSize  = OracleResultSet_getColumnSize,
        .next           = OracleResultSet_next,
        .isnull         = OracleResultSet_isnull,
        .getString      = OracleResultSet_getString,
        .getBlob        = OracleResultSet_getBlob
        // getTimestamp and getDateTime is handled in ResultSet
};
typedef struct column_t {
        OCIDefine *def;
        int isNull;
        char *buffer;
        char *name;
        unsigned long length;
        OCILobLocator *lob_loc;
        OCIDateTime   *date; 
} *column_t;
#define T ResultSetDelegate_T
struct T {
        int         columnCount;
        int         row;
        ub4         maxRow;
        OCIStmt*    stmt;
        OCIEnv*     env;
        OCISession* usr;
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
#define DATE_STR_BUF_SIZE   255


/* ------------------------------------------------------- Private methods */


static int _initaleDefiningBuffers(T R) {
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
                        case SQLT_DAT:
                        case SQLT_DATE:
                        case SQLT_TIMESTAMP:
                        case SQLT_TIMESTAMP_TZ:
                        case SQLT_TIMESTAMP_LTZ:
                                R->columns[i-1].buffer = NULL;
                                status = OCIDescriptorAlloc((dvoid *)R->env, (dvoid **) &(R->columns[i-1].date),
                                                (ub4) OCI_DTYPE_TIMESTAMP,
                                                (size_t) 0, (dvoid **) 0);
                                R->lastError = OCIDefineByPos(R->stmt, &R->columns[i-1].def, R->err, i, 
                                        &(R->columns[i-1].date), sizeof(R->columns[i-1].date), SQLT_TIMESTAMP, &(R->columns[i-1].isNull), 0, 0, OCI_DEFAULT);
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
                        char* tmp_buffer;

                        R->lastError = OCIAttrGet(pard, OCI_DTYPE_PARAM, &col_name, &col_name_len, OCI_ATTR_NAME, R->err);
                        if (R->lastError != OCI_SUCCESS)
                                continue;
                        // column name could be non NULL terminated
                        // it is not allowed to do: col_name[col_name_len] = 0;
                        // so, copy the string
                        tmp_buffer = Str_ndup(col_name, col_name_len);
#if defined(ORACLE_COLUMN_NAME_LOWERCASE) && ORACLE_COLUMN_NAME_LOWERCASE > 1
                        R->columns[i-1].name = CALLOC(1, col_name_len+1);
                        OCIMultiByteStrCaseConversion(R->env, R->columns[i-1].name, tmp_buffer, OCI_NLS_LOWERCASE);
                        FREE(tmp_buffer);
#else
                        R->columns[i-1].name = tmp_buffer;
#endif /*COLLUMN_NAME_LOWERCASE*/
                }
                OCIDescriptorFree(pard, OCI_DTYPE_PARAM);
                if (R->lastError != OCI_SUCCESS) {
                        return false;
                }
        }
        return true;
}

static int _toString(T R, int i)
{
        const char fmt[] = "IYYY-MM-DD HH24.MI.SS"; // "YYYY-MM-DD HH24:MI:SS TZR TZD"

        R->columns[i].length = DATE_STR_BUF_SIZE;
        if (R->columns[i].buffer)
                FREE(R->columns[i].buffer);

        R->columns[i].buffer = ALLOC(R->columns[i].length + 1);
        R->lastError = OCIDateTimeToText(R->usr, 
                                         R->err, 
                                         R->columns[i].date,
                                         fmt, strlen(fmt),
                                         0,
                                         NULL, 0,
                                         (ub4*)&(R->columns[i].length), (OraText *)R->columns[i].buffer);
        return ((R->lastError == OCI_SUCCESS) || (R->lastError == OCI_SUCCESS_WITH_INFO));;
}


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T OracleResultSet_new(OCIStmt *stmt, OCIEnv *env, OCISession* usr, OCIError *err, OCISvcCtx *svc, int need_free, int max_row) {
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
        R->usr  = usr;
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
        if (!_initaleDefiningBuffers(R)) {
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
                if ((*R)->columns[i].date)
                        OCIDescriptorFree((dvoid*)(*R)->columns[i].date, OCI_DTYPE_TIMESTAMP);
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


int OracleResultSet_isnull(T R, int columnIndex) {
        assert(R);
        int i = checkAndSetColumnIndex(columnIndex, R->columnCount);
        return R->columns[i].isNull;
}


const char *OracleResultSet_getString(T R, int columnIndex) {
        assert(R);
        int i = checkAndSetColumnIndex(columnIndex, R->columnCount);
        if (R->columns[i].isNull)
                return NULL;
        if (R->columns[i].date)
        {
                if (!_toString(R, i))
                {
                        THROW(SQLException, "%s", OraclePreparedStatement_getLastError(R->lastError, R->err));
                }
        }
        if (R->columns[i].buffer)
                R->columns[i].buffer[R->columns[i].length] = 0;
        return R->columns[i].buffer;
}


const void *OracleResultSet_getBlob(T R, int columnIndex, int *size) {
        assert(R);
        int i = checkAndSetColumnIndex(columnIndex, R->columnCount);
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
                        R->columns[i].buffer = RESIZE(R->columns[i].buffer, (long)(total_bytes + LOB_CHUNK_SIZE));
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
