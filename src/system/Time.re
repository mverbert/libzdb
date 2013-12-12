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



/* --------------------------------------------------------------- Private */



/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

time_t Time_toTimestamp(const char *t) {
        assert(t);
        // TODO
        return 0;
}


sqldate_t *Time_toDate(const char *t, sqldate_t *r) {
        assert(t);
        // TODO
        return r;
}


sqltime_t *Time_toTime(const char *t, sqltime_t *r) {
        assert(t);
        // TODO
        return r;
}


sqldatetime_t *Time_toDateTime(const char *t, sqldatetime_t *r) {
        assert(t);
        // TODO
        return r;
}


time_t Time_now(void) {
	struct timeval t;
	if (gettimeofday(&t, NULL) != 0)
                THROW(AssertException, "%s", System_getLastError());
	return t.tv_sec;
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

