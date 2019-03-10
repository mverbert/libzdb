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


#ifndef SQLITEDEFS_INCLUDED
#define SQLITEDEFS_INCLUDED

#include <sqlite3.h>

#include "zdb.h"

int zdb_sqlite3_step(sqlite3_stmt *pStmt) __attribute__ ((visibility("hidden")));
int zdb_sqlite3_prepare_v2(sqlite3 *db, const char *zSql, int nSql, sqlite3_stmt **ppStmt, const char **pz) __attribute__ ((visibility("hidden")));
int zdb_sqlite3_exec(sqlite3 *db, const char *sql) __attribute__ ((visibility("hidden")));

ResultSetDelegate_T SQLiteResultSet_new(Connection_T delegator, sqlite3_stmt *stmt, int keep) __attribute__ ((visibility("hidden")));
PreparedStatementDelegate_T SQLitePreparedStatement_new(Connection_T delegator, sqlite3_stmt *stmt)
 __attribute__ ((visibility("hidden")));

#endif
