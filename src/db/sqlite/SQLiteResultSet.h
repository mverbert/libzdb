/*
 * Copyright (C) 2004-2008 Tildeslash Ltd. All rights reserved.
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
#ifndef SQLITERESULTSET_INCLUDED
#define SQLITERESULTSET_INCLUDED
#define T ResultSetImpl_T
T SQLiteResultSet_new(void *stmt, int maxRows, int keep);
void SQLiteResultSet_free(T *R);
int SQLiteResultSet_getColumnCount(T R);
const char *SQLiteResultSet_getColumnName(T R, int column);
int SQLiteResultSet_next(T R);
long SQLiteResultSet_getColumnSize(T R, int columnIndex);
const char *SQLiteResultSet_getString(T R, int columnIndex);
const char *SQLiteResultSet_getStringByName(T R, const char *columnName);
int SQLiteResultSet_getInt(T R, int columnIndex);
int SQLiteResultSet_getIntByName(T R, const char *columnName);
long long int SQLiteResultSet_getLLong(T R, int columnIndex);
long long int SQLiteResultSet_getLLongByName(T R, const char *columnName);
double SQLiteResultSet_getDouble(T R, int columnIndex);
double SQLiteResultSet_getDoubleByName(T R, const char *columnName);
const void *SQLiteResultSet_getBlob(T R, int columnIndex, int *size);
const void *SQLiteResultSet_getBlobByName(T R, const char *columnName, int *size);
int SQLiteResultSet_readData(T R, int columnIndex, void *b, int l, long off);
#undef T
#endif
