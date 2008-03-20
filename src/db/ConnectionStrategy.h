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


#ifndef CONNECTIONSTRATEGY_H
#define CONNECTIONSTRATEGY_H


/**
 * This interface defines the <b>contract</b> for the concrete database 
 * implementation used for delegation in the Connection class.
 *
 * @version \$Id: ConnectionStrategy.h,v 1.20 2008/01/03 17:26:04 hauk Exp $
 * @file
 */ 

#define T IConnection_T
typedef struct T *T;

typedef struct conop {
        char *name;
	T (*new)(URL_T url, char **error);
	void (*free)(T *C);
	void (*setQueryTimeout)(T C, int ms);
        void (*setMaxRows)(T C, int max);
        int (*ping)(T C);
        int (*beginTransaction)(T C);
        int (*commit)(T C);
	int (*rollback)(T C);
	long long int (*lastRowId)(T C);
	long long int (*rowsChanged)(T C);
	int (*execute)(T C, const char *sql, va_list ap);
	ResultSet_T (*executeQuery)(T C, const char *sql, va_list ap);
        PreparedStatement_T (*prepareStatement)(T C, const char *sql);
        const char *(*getLastError)(T C);
} *Cop_T;

#undef T
#endif
