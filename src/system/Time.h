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
#include <time.h>


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
 * Returns a Unix timestamp representation of the parsed string in the
 * GMT timezone. If the given string contains timezone offset the time
 * is expected to be in local time and the offset is added to the returned
 * timestamp to make the time UTC. If the string does not contain timezone
 * information, the time is expected and assumed to be in the GTM timezone, 
 * i.e. in UTC. Example:
 * <pre>
 *  Time_toTimestamp("2013-12-15 00:12:58Z") -> 1387066378
 *  Time_toTimestamp("2013-12-14 19:12:58-05:00") -> 1387066378
 * </pre>
 * @param s The Date String to parse. Time is expected to be in UTC, but
 * local time with timezone information is also allowed.
 * @return A UTC time representation of <code>s</code> or 0 if
 * <code>s</code> is NULL
 * @exception SQLException If the parameter value cannot be converted
 * to a valid timestamp
 * @see SQLException.h
 */
time_t Time_toTimestamp(const char *s);


/**
 * Returns a Date, Time or DateTime representation of the parsed string. 
 * Fields follows the convention of the tm structure where,
 * tm_hour = hours since midnight [0-23], tm_min = minutes after the hour 
 * [0-59], tm_sec = seconds after the minute [0-60], tm_mday = day of the month
 * [1-31] and tm_mon = months since January [0-11]. tm_gmtoff is set to the 
 * offset from UTC in seconds if the time string contains timezone information,
 * otherwise tm_gmtoff is set to 0. <i>On systems without tm_gmtoff, (Solaris), 
 * the member, tm_wday is set to gmt offset instead as this property is ignored
 * by mktime on input.</i>The exception is tm_year which contains the year 
 * literal and <i>not years since 1900</i> which is the convention. All other 
 * fields in the structure are set to zero. If the given date string 
 * <code>s</code> contains both date and time all the fields mentioned above 
 * are set, otherwise only the Date or Time fields are set.
 * @param s The Date String to parse
 * @param t A pointer to a tm structure
 * @return A pointer to the tm structure representing the date of <code>s</code>
 * @exception SQLException If the parameter value cannot be converted
 * to a valid Date, Time or DateTime
 * @see SQLException.h
 */
struct tm *Time_toDateTime(const char *s, struct tm *t);


/**
 * Returns an ISO-8601 date string for the given UTC time. (The 'T' separating
 * date and time is omitted) The returned string represent the specified time 
 * in GMT timezone. The submitted result buffer must be large enough to hold at
 * least 20 bytes. Example:
 * <pre>
 *  Time_toString(1386951482, buf) -> "2013-12-13 16:18:02"
 * </pre>
 * @param time Number of time seconds since the EPOCH in UTC
 * @param result The buffer to write the date string too
 * @return a pointer to the result buffer
 * @exception AssertException if result is NULL
 */
char *Time_toString(time_t time, char result[20]);


/**
 * Returns the time since the Epoch (00:00:00 UTC, January 1, 1970),
 * measured in seconds. 
 * @return A time_t representing the system's notion of the current GMT time
 * @exception AssertException If time could not be obtained
 */
time_t Time_now(void);


/**
 * Returns the time since the Epoch (00:00:00 UTC, January 1, 1970),
 * measured in milliseconds. 
 * @return A 64 bits long representing the system's notion of the 
 * current GMT time in milliseconds
 * @exception AssertException If time could not be obtained
 */
long long Time_milli(void);


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
