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
#include <sys/time.h>
#include <sys/select.h>

#include "Str.h"
#include "system/System.h"
#include "system/Time.h"


/**
 * Implementation of the Time interface
 *
 * ISO 8601: http://en.wikipedia.org/wiki/ISO_8601
 * RFC 3339: http://tools.ietf.org/html/rfc3339
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#define i2a(i) (x[0] = ((i) / 10) + '0', x[1] = ((i) % 10) + '0')


/* --------------------------------------------------------------- Private */


static inline int a2i(const char *a, int l) {
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
                struct tm t = {0};
                if (Time_toDateTime(s, &t)) {
                        t.tm_year -= 1900;
                        long off = t.tm_gmtoff;
                        return mktime(&t) - off; // mktime does not honor tm_gmtoff
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
                 
                 yyyy x dd x dd
                 { // Date: YYYY-MM-DD
                        tm.tm_year  = a2i(token, 4);
                        tm.tm_mon   = a2i(token + 5, 2) - 1;
                        tm.tm_mday  = a2i(token + 8, 2);
                        has_date = true;
                        continue;
                 }
                 yyyy dd dd
                 { // Compressed Date: YYYYMMDD
                        tm.tm_year  = a2i(token, 4);
                        tm.tm_mon   = a2i(token + 4, 2) - 1;
                        tm.tm_mday  = a2i(token + 6, 2);
                        has_date = true;
                        continue;
                 }
                 dd x dd x dd
                 { // Time: HH:MM:SS
                        tm.tm_hour = a2i(token, 2);
                        tm.tm_min  = a2i(token + 3, 2);
                        tm.tm_sec  = a2i(token + 6, 2);
                        has_time = true;
                        continue;
                 }
                 dd dd dd tz?
                 { // Compressed Time: HHMMSS
                        tm.tm_hour = a2i(token, 2);
                        tm.tm_min  = a2i(token + 2, 2);
                        tm.tm_sec  = a2i(token + 4, 2);
                        has_time = true;
                        continue;
                 }
                 tz
                 { // Timezone: +-HH:MM, +-HH or +-HHMM is offset from UTC in seconds
                        if (has_time) { // Only set timezone if time has been seen
                                tm.tm_gmtoff = a2i(token + 1, 2) * 3600;
                                if (token[3] >= '0' && token[3] <= '9')
                                        tm.tm_gmtoff += a2i(token + 3, 2) * 60;
                                else if (token[4] >= '0' && token[4] <= '9')
                                        tm.tm_gmtoff += a2i(token + 4, 2) * 60;
                                if (token[0] == '+')
                                        tm.tm_gmtoff *= -1;
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


time_t Time_now(void) {
	struct timeval t;
	if (gettimeofday(&t, NULL) != 0)
                THROW(AssertException, "%s", System_getLastError());
	return t.tv_sec;
}


char *Time_toString(time_t time, char *result) {
        assert(result);
        assert(time >= 0);
        char x[2];
        struct tm ts = {0};
        localtime_r(&time, &ts);
        memcpy(result, "YYYY-MM-DD HH:MM:SS\0", 19);
        /*              0    5  8  11 14 17 */
        i2a((ts.tm_year+1900)/100);
        result[0] = x[0];
        result[1] = x[1];
        i2a((ts.tm_year+1900)%100);
        result[2] = x[0];
        result[3] = x[1];
        i2a(ts.tm_mon + 1); // Months in 01-12
        result[5] = x[0];
        result[6] = x[1];
        i2a(ts.tm_mday);
        result[8] = x[0];
        result[9] = x[1];
        i2a(ts.tm_hour);
        result[11] = x[0];
        result[12] = x[1];
        i2a(ts.tm_min);
        result[14] = x[0];
        result[15] = x[1];
        i2a(ts.tm_sec);
        result[17] = x[0];
        result[18] = x[1];
	return result;
}


long long int Time_milli(void) {
	struct timeval t;
	if (gettimeofday(&t, NULL) != 0)
                THROW(AssertException, "%s", System_getLastError());
	return (long long int)t.tv_sec * 1000  +  (long long int)t.tv_usec / 1000;
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

