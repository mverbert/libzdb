/*
 * Copyright (C) 2004-2009 Tildeslash Ltd. All rights reserved.
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


#ifndef UTIL_INCLUDED
#define UTIL_INCLUDED
#include <stdarg.h>


/**
 * General purpose utility <b>class methods</b>.
 *
 * @file
 */


/**
 * Returns the time since the Epoch (00:00:00 UTC, January 1, 1970),
 * measured in seconds.
 * @return The current time since the Epoch in seconds
 * @exception AssertException If time could not be obtained
 */
long Util_seconds();


/**
 * This method suspend the calling process or Thread for
 * <code>u</code> micro seconds.
 * @param u Micro seconds to sleep
 * @return The number of micro seconds slept
 */
long Util_usleep(long u);


/**
 * Print a formated message to stdout
 * @param e A formated (printf-style) message string
 */
void Util_debug(const char *e, ...);


/**
 * Prints the given error message to <code>stderr</code> and 
 * <code>abort(3)</code> the application. If an AbortHandler callback 
 * function is defined for the library, this function is called instead.
 * @param e A formated (printf-style) message string
 */
void Util_abort(const char *e, ...);


#endif
