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


#ifndef PREPAREDSTATEMENTSTRATEGY_H
#define PREPAREDSTATEMENTSTRATEGY_H


/**
 * This interface defines the <b>contract</b> for the concrete database 
 * implementation used for delegation in the PreparedStatement class.
 *
 * @version \$Id: PreparedStatementStrategy.h,v 1.11 2008/01/03 17:26:05 hauk Exp $
 * @file
 */ 

#define T IPreparedStatement_T
typedef struct T *T;

typedef struct pop {
	char *name;
        void (*free)(T *P);
        int (*setString)(T P, int parameterIndex, const char *x);
        int (*setInt)(T P, int parameterIndex, int x);
        int (*setLLong)(T P, int parameterIndex, long long int x);
        int (*setDouble)(T P, int parameterIndex, double x);
        int (*setBlob)(T P, int parameterIndex, const void *x, int size);
        int (*execute)(T P);
        ResultSet_T (*executeQuery)(T P);
} *Pop_T;

#undef T
#endif
