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


#include "Config.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "Str.h"
#include "system/System.h"


/**
 * Implementation of the System Facade for UNIX Systems.
 *
 * @file
 */
 

/* ----------------------------------------------------------- Definitions */


extern void(*AbortHandler)(const char *error);


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

const char *System_getLastError(void) { 
        return strerror(errno); 
}


const char *System_getError(int error) { 
        return strerror(error); 
}


void System_abort(const char *e, ...) {
	va_list ap;
	va_start(ap, e);
	if (AbortHandler) {
	        char *t = Str_vcat(e, ap);
	        AbortHandler(t); 
                FREE(t);
	} else {
                vfprintf(stderr, e, ap);
                if (ZBDEBUG)
                        abort();
                else
                        exit(1);
	}
	va_end(ap);
}


void System_debug(const char *s, ...) {
        if (ZBDEBUG) {
                va_list ap;
                va_start(ap, s);
                vfprintf(stdout, s, ap);
                va_end(ap);
        }
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif

