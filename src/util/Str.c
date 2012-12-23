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
#include <strings.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>


/**
 * Implementation of the Str interface
 *
 * @file
 */


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

int Str_isEqual(const char *a, const char *b) {
	if (a && b) { 
                while (*a && *b)
                        if (toupper(*a++) != toupper(*b++)) return false;
                return (*a == *b);
        }
        return false;
}


int Str_isByteEqual(const char *a, const char *b) {
	if (a && b) {
                while (*a && *b)
                        if (*a++ != *b++) return false;
                return (*a == *b);
        }
        return false;
}


int Str_startsWith(const char *a, const char *b) {
	if (a && b) {
	        do 
	                if (*a++ != *b++) return false;
                while (*b);
                return true;
        }
        return false;
}


char *Str_copy(char *dest, const char *src, int n) {
	if (src && dest && (n > 0)) { 
        	char *t = dest;
	        while (*src && n--)
        		*t++ = *src++;
        	*t = 0;
	} else if (dest)
	        *dest = 0;
        return dest;
}


// We do not use strdup so we can throw MemoryException on OOM
char *Str_dup(const char *s) { 
        char *t = NULL;
        if (s) {
                size_t n = strlen(s); 
                t = ALLOC(n + 1);
                memcpy(t, s, n);
                t[n] = 0;
        }
        return t;
}


char *Str_ndup(const char *s, int n) {
        char *t = NULL;
        assert(n >= 0);
        if (s) {
                int l = (int)strlen(s); 
                n = l < n ? l : n; // Use the actual length of s if shorter than n
                t = ALLOC(n + 1);
                memcpy(t, s, n);
                t[n] = 0;
        }
        return t;
}


char *Str_cat(const char *s, ...) {
	char *t = 0;
	if (s) {
                va_list ap;
                va_start(ap, s);
                t = Str_vcat(s, ap);
                va_end(ap);
        }
	return t;
}


char *Str_vcat(const char *s, va_list ap) {
        char *buf = NULL;
        if (s) {
                int n = 0;
                va_list ap_copy;
                int size = STRLEN;
                buf = ALLOC(size);
                while (true) {
                        va_copy(ap_copy, ap);
                        n = vsnprintf(buf, size, s, ap_copy);
                        va_end(ap_copy);
                        if (n < size)
                                break;
                        size = n + 1;
                        RESIZE(buf, size);
                }
        }
        return buf;
}


int Str_parseInt(const char *s) {
	if (STR_UNDEF(s))
		THROW(SQLException, "NumberFormatException: For input string null");
        errno = 0;
        char *e;
	int i = (int)strtol(s, &e, 10);
	if (errno || (e == s))
		THROW(SQLException, "NumberFormatException: For input string %s -- %s", s, System_getLastError());
	return i;
}


long long int Str_parseLLong(const char *s) {
	if (STR_UNDEF(s))
		THROW(SQLException, "NumberFormatException: For input string null");
        errno = 0;
        char *e;
	long long l = strtoll(s, &e, 10);
	if (errno || (e == s))
		THROW(SQLException, "NumberFormatException: For input string %s -- %s", s, System_getLastError());
	return l;
}


double Str_parseDouble(const char *s) {
	if (STR_UNDEF(s))
		THROW(SQLException, "NumberFormatException: For input string null");
        errno = 0;
        char *e;
	double d = strtod(s, &e);
	if (errno || (e == s))
		THROW(SQLException, "NumberFormatException: For input string %s -- %s", s, System_getLastError());
	return d;
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif

