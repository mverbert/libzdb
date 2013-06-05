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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.
 */
#ifndef ORACLE_PREPAREDSTATEMENT_INCLUDED
#define ORACLE_PREPAREDSTATEMENT_INCLUDED
#define T PreparedStatementDelegate_T
T OraclePreparedStatement_new(OCIStmt *stmt, OCIEnv *env, OCIError *err, OCISvcCtx *svc, int max_row);
void OraclePreparedStatement_free(T *P);
void OraclePreparedStatement_setString(T P, int parameterIndex, const char *x);
void OraclePreparedStatement_setInt(T P, int parameterIndex, int x);
void OraclePreparedStatement_setLLong(T P, int parameterIndex, long long int x);
void OraclePreparedStatement_setDouble(T P, int parameterIndex, double x);
void OraclePreparedStatement_setBlob(T P, int parameterIndex, const void *x, int size);
void OraclePreparedStatement_execute(T P);
ResultSet_T OraclePreparedStatement_executeQuery(T P);
const char *OraclePreparedStatement_getLastError(int err, OCIError *errhp);
#undef T
#endif
