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

#ifndef ORACLE_PREPAREDSTATEMENT_INCLUDED
#define ORACLE_PREPAREDSTATEMENT_INCLUDED
#define T PreparedStatementDelegate_T
T OraclePreparedStatement_new(OCIStmt *stmt, OCIEnv *env, OCISession* usr, OCIError *err, OCISvcCtx *svc, int max_row);
void OraclePreparedStatement_free(T *P);
void OraclePreparedStatement_setString(T P, int parameterIndex, const char *x);
void OraclePreparedStatement_setInt(T P, int parameterIndex, int x);
void OraclePreparedStatement_setLLong(T P, int parameterIndex, long long x);
void OraclePreparedStatement_setDouble(T P, int parameterIndex, double x);
void OraclePreparedStatement_setBlob(T P, int parameterIndex, const void *x, int size);
void OraclePreparedStatement_setTimestamp(T P, int parameterIndex, time_t time);
void OraclePreparedStatement_execute(T P);
ResultSet_T OraclePreparedStatement_executeQuery(T P);
long long OraclePreparedStatement_rowsChanged(T P);
const char *OraclePreparedStatement_getLastError(int err, OCIError *errhp);
#undef T
#endif
