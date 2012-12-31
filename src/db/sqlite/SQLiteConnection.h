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
#ifndef SQLITECONNECTION_INCLUDED
#define SQLITECONNECTION_INCLUDED
#define T ConnectionDelegate_T
T SQLiteConnection_new(URL_T url, char **error);
void SQLiteConnection_free(T *C);
void SQLiteConnection_setQueryTimeout(T C, int ms);
void SQLiteConnection_setMaxRows(T C, int max);
int SQLiteConnection_ping(T C);
int SQLiteConnection_beginTransaction(T C);
int SQLiteConnection_commit(T C);
int SQLiteConnection_rollback(T C);
long long int SQLiteConnection_lastRowId(T C);
long long int SQLiteConnection_rowsChanged(T C);
int SQLiteConnection_execute(T C, const char *sql, va_list ap);
ResultSet_T SQLiteConnection_executeQuery(T C, const char *sql, va_list ap);
PreparedStatement_T SQLiteConnection_prepareStatement(T C, const char *sql, va_list ap);
const char *SQLiteConnection_getLastError(T C);
/* Event handlers */
void SQLiteConnection_onstop(void);
#undef T
#endif

