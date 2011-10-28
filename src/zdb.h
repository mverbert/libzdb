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


#ifndef ZDB_INCLUDED
#define ZDB_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif


/**
 * Include this interface in your C code to import the libzdb API.
 * This meta interface also wraps libzdb API interface files in an
 * 'extern "C"' block to allow for including libzdb in a C++ or in
 * a Objective-C++ project.
 *
 * @file
 */

/* Mask out __attribute__ extension for non- GCC/llvm-clang compilers. */
#if (! (defined(__GNUC__) || defined(__clang__)))
#define __attribute__(x)
#endif

/* libzdb API interfaces */
#include <URL.h>
#include <ResultSet.h>
#include <PreparedStatement.h>
#include <Connection.h>
#include <ConnectionPool.h>
#include <SQLException.h>


#ifdef __cplusplus
}
#endif
#endif
