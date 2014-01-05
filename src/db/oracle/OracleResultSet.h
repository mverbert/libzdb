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

#ifndef ORACLE_RESULTSET_INCLUDED
#define ORACLE_RESULTSET_INCLUDED
#define T ResultSetDelegate_T
T OracleResultSet_new(OCIStmt* stmt, OCIEnv* env, OCISession* usr, OCIError* err, OCISvcCtx* svc, int need_free, int max_row);
void OracleResultSet_free(T *R);
int  OracleResultSet_getColumnCount(T R);
const char *OracleResultSet_getColumnName(T R, int columnIndex);
long OracleResultSet_getColumnSize(T R, int columnIndex);
int  OracleResultSet_next(T R);
int OracleResultSet_isnull(T R, int columnIndex);
const char *OracleResultSet_getString(T R, int columnIndex);
const void *OracleResultSet_getBlob(T R, int columnIndex, int *size);
#undef T
#endif
