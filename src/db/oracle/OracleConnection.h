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
#ifndef ORACLE_CONNECTION_INCLUDED
#define ORACLE_CONNECTION_INCLUDED
#define T ConnectionDelegate_T
T    OracleConnection_new(URL_T url, char **error);
void OracleConnection_free(T *C);
void OracleConnection_setQueryTimeout(T C, int ms);
void OracleConnection_setMaxRows(T C, int max);
int  OracleConnection_ping(T C);
int  OracleConnection_beginTransaction(T C);
int  OracleConnection_commit(T C);
int  OracleConnection_rollback(T C);
long long OracleConnection_lastRowId(T C);
long long OracleConnection_rowsChanged(T C);
int  OracleConnection_execute(T C, const char *sql, va_list ap);
ResultSet_T OracleConnection_executeQuery(T C, const char *sql, va_list ap);
PreparedStatement_T OracleConnection_prepareStatement(T C, const char *sql, va_list ap);
const char *OracleConnection_getLastError(T C);
#undef T
#endif
