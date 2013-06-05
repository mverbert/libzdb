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


#ifndef CONFIG_INCLUDED
#define CONFIG_INCLUDED


/**
 * Global defines, macros and types
 *
 * @file
 */


#include <assert.h>
#include <errno.h>
#include "xconfig.h"

#include "system/Mem.h"
#include "Str.h"
#include "SQLException.h"
#include "system/System.h"


/**
 * The libzdb URL
 */
#define LIBZDB_URL	"http://tildeslash.com/libzdb/"


/**
 * Version, copyright and contact information
 */
#define ABOUT   "libzdb/" VERSION " Copyright (C) Tildeslash Ltd. " LIBZDB_URL


/* ----------------------------------- Error, Exceptions and report macros */


/**
 * The standard abort routine
 */
#define ABORT	System_abort


/**
 * The standard debug routine
 */
#define DEBUG	System_debug


/* --------------------------------------------- SQL standard value macros */


/**
 * Standard millisecond timeout value for a database call. 
 */
#define SQL_DEFAULT_TIMEOUT 3000


/**
 * The default maximum number of database connections
 */
#define SQL_DEFAULT_MAX_CONNECTIONS 20


/**
 * The initial number of database connections
 */
#define SQL_DEFAULT_INIT_CONNECTIONS 5


/**
 * The standard sweep interval in seconds for a ConnectionPool reaper thread
 */
#define SQL_DEFAULT_SWEEP_INTERVAL 60


/**
 * Default Connection timeout in seconds, used by reaper to remove
 * inactive connections
 */
#define SQL_DEFAULT_CONNECTION_TIMEOUT 30


/**
 * Default TCP/IP Connection timeout in seconds, used when connecting to
 * a database server over a TCP/IP connection
 */
#define SQL_DEFAULT_TCP_TIMEOUT 3


/**
 * MySQL default server port number
 */
#define MYSQL_DEFAULT_PORT 3306


/**
 * PostgreSQL default server port number
 */
#define POSTGRESQL_DEFAULT_PORT 5432


/**
 * Oracle default server port number
 */
#define ORACLE_DEFAULT_PORT 1521


/* ------------------------------------------ General Purpose value macros */


/**
 * Standard String length
 */
#define STRLEN 256


/**
 * Boolean truth value
 */
#define true 1


/**
 * Boolean false value
 */
#define false 0


/** 
 * Microseconds per second 
*/
#define USEC_PER_SEC 1000000


/** 
 * Microseconds per millisecond 
 */
#define USEC_PER_MSEC 1000


/* ------------------------------------- General Purpose functional macros */


#define IS      Str_isEqual


/* ---------------------------------------------------------- Build macros */


/* Mask out GCC __attribute__ extension for non-gcc compilers. */
#ifndef __GNUC__
#define __attribute__(x)
#endif


/* ------------------------------------------------------ Type definitions */


/**
 * The internal 8-bit char type
 */
typedef unsigned char uchar_t;


/**
 * The internal 32 bits integer type
 */
typedef  unsigned int uint32_t;


/* -------------------------------------------------------------- Globals  */


/**
 * Abort handler callback
 */
extern void(*AbortHandler)(const char *error);


/**
 * Library Debug flag. If set to true, emit debug output 
 */
extern int ZBDEBUG;


#endif


