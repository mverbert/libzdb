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
#ifndef MYSQLCONNECTION_INCLUDED
#define MYSQLCONNECTION_INCLUDED
#define T ConnectionDelegate_T
T MysqlConnection_new(URL_T url, char **error);
void MysqlConnection_free(T *C);
void MysqlConnection_setQueryTimeout(T C, int ms);
void MysqlConnection_setMaxRows(T C, int max);
int MysqlConnection_ping(T C);
int MysqlConnection_beginTransaction(T C);
int MysqlConnection_commit(T C);
int MysqlConnection_rollback(T C);
long long int MysqlConnection_lastRowId(T C);
long long int MysqlConnection_rowsChanged(T C);
int MysqlConnection_execute(T C, const char *sql, va_list ap);
ResultSet_T MysqlConnection_executeQuery(T C, const char *sql, va_list ap);
PreparedStatement_T MysqlConnection_prepareStatement(T C, const char *sql, va_list ap);
const char *MysqlConnection_getLastError(T C);
/* Event handlers */
void MysqlConnection_onstop(void);
#undef T
#endif

