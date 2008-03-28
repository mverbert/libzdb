/*
 * Copyright (C) 2008 Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef POSTGRESQLRESULTSET_INCLUDED
#define POSTGRESQLRESULTSET_INCLUDED
#define T IResultSet_T
T PostgresqlResultSet_new(void *stmt, int maxRows, int keep);
void PostgresqlResultSet_free(T *R);
int PostgresqlResultSet_getColumnCount(T R);
const char *PostgresqlResultSet_getColumnName(T R, int column);
int PostgresqlResultSet_next(T R);
long PostgresqlResultSet_getColumnSize(T R, int columnIndex);
const char *PostgresqlResultSet_getString(T R, int columnIndex);
const char *PostgresqlResultSet_getStringByName(T R, const char *columnName);
int PostgresqlResultSet_getInt(T R, int columnIndex);
int PostgresqlResultSet_getIntByName(T R, const char *columnName);
long long int PostgresqlResultSet_getLLong(T R, int columnIndex);
long long int PostgresqlResultSet_getLLongByName(T R, const char *columnName);
double PostgresqlResultSet_getDouble(T R, int columnIndex);
double PostgresqlResultSet_getDoubleByName(T R, const char *columnName);
const void *PostgresqlResultSet_getBlob(T R, int columnIndex, int *size);
const void *PostgresqlResultSet_getBlobByName(T R, const char *columnName, int *size);
int PostgresqlResultSet_readData(T R, int columnIndex, void *b, int l, long off);
#undef T
#endif
