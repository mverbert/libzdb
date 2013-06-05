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


#ifndef TIME_INCLUDED
#define TIME_INCLUDED


/**
 * <b>Time</b> is an abstraction of date and time. Time is stored internally 
 * as the number of seconds and microseconds since the epoch, <i>January 1, 
 * 1970 00:00 UTC</i>. 
 *
 * @file
 */


/** @name Class methods */
//@{ 

/**
 * Returns the time since the Epoch (00:00:00 UTC, January 1, 1970),
 * measured in seconds. 
 * @return A time_t representing the current local time since the epoch
 * @exception AssertException If time could not be obtained
 */
time_t Time_now(void);


/**
 * Returns the time since the Epoch (00:00:00 UTC, January 1, 1970),
 * measured in milliseconds. 
 * @return A 64 bits long representing the current local time since 
 * the epoch in milliseconds
 * @exception AssertException If time could not be obtained
 */
long long int Time_milli(void);


/**
 * This method suspend the calling process or Thread for
 * <code>u</code> micro seconds.
 * @param u Micro seconds to sleep
 * @return true
 */
int Time_usleep(long u);

//@}

#undef T
#endif
