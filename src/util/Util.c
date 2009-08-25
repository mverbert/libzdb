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


#include "Config.h"

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>


/**
 * Implementation of the Util interface
 *
 * @file
 */


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

long Util_seconds(void) {
	struct timeval tv = {0};
	int r = gettimeofday(&tv, 0);
        assert(r == 0);
	return tv.tv_sec;
}


long Util_usleep(long u) {
        struct timeval tv;
        tv.tv_sec = u / USEC_PER_SEC;
        tv.tv_usec = u % USEC_PER_SEC;
        select(0, 0, 0, 0, &tv);
        return u;
}


void Util_debug(const char *s, ...) {
	va_list ap;
	va_start(ap, s);
	vfprintf(stdout, s, ap);
	va_end(ap);
}


void Util_abort(const char *e, ...) {
	va_list ap;
        uchar_t buf[ERROR_SIZE + 1];
	va_start(ap, e);
	vsnprintf(buf, ERROR_SIZE, e, ap);
	va_end(ap);
        if (! AbortHandler) {
                fprintf(stderr, "%s", buf);
                abort();
        } 
        AbortHandler(buf); 
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif
