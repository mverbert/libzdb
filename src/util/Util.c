/*
 * Copyright (C) 2008 Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
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

#include "Util.h"


/**
 * Implementation of the Util interface
 *
 *  @version \$Id: Util.c,v 1.27 2008/03/20 11:28:54 hauk Exp $
 *  @file
 */


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif


long Util_seconds() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec;
}


long Util_usleep(long u) {
        struct timeval tv;
        tv.tv_sec = u / USEC_PER_SEC;
        tv.tv_usec = u % USEC_PER_SEC;
        select(0, NULL, NULL, NULL, &tv);
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
        if (AbortHandler == NULL) {
                fprintf(stderr, buf);
                abort();
        } 
        AbortHandler(buf); 
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif
