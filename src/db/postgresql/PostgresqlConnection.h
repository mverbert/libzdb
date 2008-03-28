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
#ifndef POSTGRESQLCONNECTION_INCLUDED
#define POSTGRESQLCONNECTION_INCLUDED
#define T IConnection_T
T PostgresqlConnection_new(URL_T url, char **error);
void PostgresqlConnection_free(T *C);
void PostgresqlConnection_setQueryTimeout(T C, int ms);
void PostgresqlConnection_setMaxRows(T C, int max);
int PostgresqlConnection_ping(T C);
int PostgresqlConnection_beginTransaction(T C);
int PostgresqlConnection_commit(T C);
int PostgresqlConnection_rollback(T C);
long long int PostgresqlConnection_lastRowId(T C);
long long int PostgresqlConnection_rowsChanged(T C);
int PostgresqlConnection_execute(T C, const char *sql, va_list ap);
ResultSet_T PostgresqlConnection_executeQuery(T C, const char *sql, va_list ap);
PreparedStatement_T PostgresqlConnection_prepareStatement(T C, const char *sql);
const char *PostgresqlConnection_getLastError(T C);
#undef T
#endif

