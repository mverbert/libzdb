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
long long int OracleConnection_lastRowId(T C);
long long int OracleConnection_rowsChanged(T C);
int  OracleConnection_execute(T C, const char *sql, va_list ap);
ResultSet_T OracleConnection_executeQuery(T C, const char *sql, va_list ap);
PreparedStatement_T OracleConnection_prepareStatement(T C, const char *sql, va_list ap);
const char *OracleConnection_getLastError(T C);
/* Event handlers */
void OracleConnection_onstop(void);
#undef T
#endif
