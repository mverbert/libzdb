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


#ifndef PREPAREDSTATEMENTDELEGATE_INCLUDED
#define PREPAREDSTATEMENTDELEGATE_INCLUDED

/**
 * This interface defines the <b>contract</b> for the concrete database 
 * implementation used for delegation in the PreparedStatement class.
 *
 * @file
 */ 

#define T PreparedStatementDelegate_T
typedef struct T *T;

typedef struct Pop_T {
	const char *name;
        void (*free)(T *P);
        void (*setString)(T P, int parameterIndex, const char *x);
        void (*setInt)(T P, int parameterIndex, int x);
        void (*setLLong)(T P, int parameterIndex, long long x);
        void (*setDouble)(T P, int parameterIndex, double x);
        void (*setTimestamp)(T P, int parameterIndex, time_t timestamp);
        void (*setBlob)(T P, int parameterIndex, const void *x, int size);
        void (*execute)(T P);
        ResultSet_T (*executeQuery)(T P);
        long long (*rowsChanged)(T P);
} *Pop_T;

/**
 * Throws exception if parameterIndex is outside the parameterCount range.
 * @return parameterIndex - 1. In the API parameterIndex starts with 1,
 * internally it starts with 0.
 */
static inline int checkAndSetParameterIndex(int parameterIndex, int parameterCount) {
        int i = parameterIndex - 1;
        if (parameterCount <= 0 || i < 0 || i >= parameterCount)
                THROW(SQLException, "Parameter index is out of range");
        return i;
}

#undef T
#endif
