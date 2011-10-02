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
#ifndef ORACLE_RESULTSET_INCLUDED
#define ORACLE_RESULTSET_INCLUDED
#define T ResultSetDelegate_T
T OracleResultSet_new(OCIStmt* stmt, OCIEnv* env, OCIError* err, OCISvcCtx* svc, int need_free, int max_row);
void OracleResultSet_free(T *R);
int  OracleResultSet_getColumnCount(T R);
const char *OracleResultSet_getColumnName(T R, int column);
int  OracleResultSet_next(T R);
long OracleResultSet_getColumnSize(T R, int columnIndex);
const char *OracleResultSet_getString(T R, int columnIndex);
const char *OracleResultSet_getStringByName(T R, const char *columnName);
int OracleResultSet_getInt(T R, int columnIndex);
int OracleResultSet_getIntByName(T R, const char *columnName);
long long int OracleResultSet_getLLong(T R, int columnIndex);
long long int OracleResultSet_getLLongByName(T R, const char *columnName);
double OracleResultSet_getDouble(T R, int columnIndex);
double OracleResultSet_getDoubleByName(T R, const char *columnName);
const void *OracleResultSet_getBlob(T R, int columnIndex, int *size);
const void *OracleResultSet_getBlobByName(T R, const char *columnName, int *size);
#undef T
#endif
