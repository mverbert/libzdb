/*
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
#ifndef POSTGRESQLCONNECTION_INCLUDED
#define POSTGRESQLCONNECTION_INCLUDED
#define T ConnectionDelegate_T
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
PreparedStatement_T PostgresqlConnection_prepareStatement(T C, const char *sql, va_list ap);
const char *PostgresqlConnection_getLastError(T C);
/* Event handlers */
void  PostgresqlConnection_onstop(void);
#undef T
#endif

