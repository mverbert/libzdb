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


#include "Config.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <limits.h>

#include "Str.h"
#include "system/System.h"
#include "system/Time.h"


/**
 * Implementation of the Time interface
 *
 * ISO 8601: http://en.wikipedia.org/wiki/ISO_8601
 * @file
 */


/* ----------------------------------------------------------- Definitions */

#ifndef HAVE_TIMEGM
/*
 * Spdylay - SPDY Library
 *
 * Copyright (c) 2013 Tatsuhiro Tsujikawa
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/* Counter the number of leap year in the range [0, y). The |y| is the
 year, including century (e.g., 2012) */
static int count_leap_year(int y)
{
        y -= 1;
        return y/4-y/100+y/400;
}


/* Returns nonzero if the |y| is the leap year. The |y| is the year,
 including century (e.g., 2012) */
static int is_leap_year(int y)
{
        return y%4 == 0 && (y%100 != 0 || y%400 == 0);
}


/* The number of days before ith month begins */
static int daysum[] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};


/* Based on the algorithm of Python 2.7 calendar.timegm. */
time_t timegm(struct tm *tm)
{
        int days;
        int num_leap_year;
        int64_t t;
        if(tm->tm_mon > 11) {
                return -1;
        }
        num_leap_year = count_leap_year(tm->tm_year + 1900) - count_leap_year(1970);
        days = (tm->tm_year - 70) * 365 +
        num_leap_year + daysum[tm->tm_mon] + tm->tm_mday-1;
        if(tm->tm_mon >= 2 && is_leap_year(tm->tm_year + 1900)) {
                ++days;
        }
        t = ((int64_t)days * 24 + tm->tm_hour) * 3600 + tm->tm_min * 60 + tm->tm_sec;
        if(sizeof(time_t) == 4) {
                if(t < INT_MIN || t > INT_MAX) {
                        return -1;
                }
        }
        return t;
}
#endif /* !HAVE_TIMEGM */

#if HAVE_STRUCT_TM_TM_GMTOFF
#define TM_GMTOFF tm_gmtoff
#else
#define TM_GMTOFF tm_wday
#endif

#define _i2a(i) (x[0] = ((i) / 10) + '0', x[1] = ((i) % 10) + '0')


/* --------------------------------------------------------------- Private */


static inline int _a2i(const char *a, int l) {
        int n = 0;
        for (; *a && l--; a++)
                n = n * 10 + (*a) - '0';
        return n;
}


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif


time_t Time_toTimestamp(const char *s) {
        if (STR_DEF(s)) {
                struct tm t = {};
                if (Time_toDateTime(s, &t)) {
                        t.tm_year -= 1900;
                        time_t offset = t.TM_GMTOFF;
                        return timegm(&t) - offset;
                }
        }
	return 0;
}


struct tm *Time_toDateTime(const char *s, struct tm *t) {
        assert(t);
        assert(s);
        struct tm tm = {.tm_isdst = -1}; 
        int has_date = false, has_time = false;
        const char *limit = s + strlen(s), *marker, *token, *cursor = s;
	while (true) {
		if (cursor >= limit) {
                        if (has_date || has_time) {
                                *(struct tm*)t = tm;
                                return t;
                        }
                        THROW(SQLException, "Invalid date or time");
                }
                token = cursor;
                /*!re2c
                 re2c:define:YYCTYPE  = "unsigned char";
                 re2c:define:YYCURSOR = cursor;
                 re2c:define:YYLIMIT  = limit;
                 re2c:define:YYMARKER = marker;
                 re2c:yyfill:enable   = 0;
                 
                 any    = [\000-\377];
                 x      = [^0-9];
                 dd     = [0-9][0-9];
                 yyyy   = [0-9]{4};
                 tz     = [-+]dd(.? dd)?;
                 frac   = [.,][0-9]+;
                 
                 yyyy x dd x dd
                 { // Date: YYYY-MM-DD
                        tm.tm_year  = _a2i(token, 4);
                        tm.tm_mon   = _a2i(token + 5, 2) - 1;
                        tm.tm_mday  = _a2i(token + 8, 2);
                        has_date = true;
                        continue;
                 }
                 yyyy dd dd
                 { // Compressed Date: YYYYMMDD
                        tm.tm_year  = _a2i(token, 4);
                        tm.tm_mon   = _a2i(token + 4, 2) - 1;
                        tm.tm_mday  = _a2i(token + 6, 2);
                        has_date = true;
                        continue;
                 }
                 dd x dd x dd frac?
                 { // Time: HH:MM:SS
                        tm.tm_hour = _a2i(token, 2);
                        tm.tm_min  = _a2i(token + 3, 2);
                        tm.tm_sec  = _a2i(token + 6, 2);
                        has_time = true;
                        continue;
                 }
                 dd dd dd frac?
                 { // Compressed Time: HHMMSS
                        tm.tm_hour = _a2i(token, 2);
                        tm.tm_min  = _a2i(token + 2, 2);
                        tm.tm_sec  = _a2i(token + 4, 2);
                        has_time = true;
                        continue;
                 }
                 tz
                 { // Timezone: +-HH:MM, +-HH or +-HHMM is offset from UTC in seconds
                        if (has_time) { // Only set timezone if time has been seen
                                tm.TM_GMTOFF = _a2i(token + 1, 2) * 3600;
                                if (isdigit(token[3]))
                                        tm.TM_GMTOFF += _a2i(token + 3, 2) * 60;
                                else if (isdigit(token[4]))
                                        tm.TM_GMTOFF += _a2i(token + 4, 2) * 60;
                                if (token[0] == '-')
                                        tm.TM_GMTOFF *= -1;
                        }
                        continue;
                 }
                 any
                 {
                        continue;
                 }
                 */
        }
	return NULL;
}


char *Time_toString(time_t time, char result[20]) {
        assert(result);
        char x[2];
        struct tm ts = {.tm_isdst = -1};
        gmtime_r(&time, &ts);
        memcpy(result, "YYYY-MM-DD HH:MM:SS\0", 20);
        /*              0    5  8  11 14 17 */
        _i2a((ts.tm_year+1900)/100);
        result[0] = x[0];
        result[1] = x[1];
        _i2a((ts.tm_year+1900)%100);
        result[2] = x[0];
        result[3] = x[1];
        _i2a(ts.tm_mon + 1); // Months in 01-12
        result[5] = x[0];
        result[6] = x[1];
        _i2a(ts.tm_mday);
        result[8] = x[0];
        result[9] = x[1];
        _i2a(ts.tm_hour);
        result[11] = x[0];
        result[12] = x[1];
        _i2a(ts.tm_min);
        result[14] = x[0];
        result[15] = x[1];
        _i2a(ts.tm_sec);
        result[17] = x[0];
        result[18] = x[1];
	return result;
}


time_t Time_now(void) {
	struct timeval t;
	if (gettimeofday(&t, NULL) != 0)
                THROW(AssertException, "%s", System_getLastError());
	return t.tv_sec;
}


long long Time_milli(void) {
	struct timeval t;
	if (gettimeofday(&t, NULL) != 0)
                THROW(AssertException, "%s", System_getLastError());
	return (long long)t.tv_sec * 1000  +  (long long)t.tv_usec / 1000;
}


int Time_usleep(long u) {
        struct timeval t;
        t.tv_sec = u / USEC_PER_SEC;
        t.tv_usec = (suseconds_t)(u % USEC_PER_SEC);
        select(0, 0, 0, 0, &t);
        return true;
}


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif
