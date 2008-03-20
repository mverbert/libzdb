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
#include <ctype.h>
#include <stdlib.h>

#include "Mem.h"
#include "Str.h"


/**
 * Implementation of the Str interface
 *
 *  @version \$Id: Str.c,v 1.1 2008/03/20 11:28:54 hauk Exp $
 *  @file
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
                const char *s = a;
                while (*a && *b)
                        if (*a++ != *b++) return false;
                return ((*a == *b) || (a != s && *b==0));
        }
        return false;
}


char *Str_copy(char *dest, const char *src, int n) {
	char *p = dest;
	if (! (src && dest)) { 
		if (dest) 
			*dest = 0; 
		return dest; 
	}
	for (; (*src && n--); src++, p++)
		*p = *src;
	*p = 0;
	return dest;
}


char *Str_dup(const char *s) {
        return (s ? Str_ndup(s, strlen(s)) : NULL);
}


char *Str_ndup(const char *s, int n) {
        char *t = NULL;
	if (s) {
		t = ALLOC(n + 1);
		memcpy(t, s, n);
		t[n] = 0;
	}
        return t;
}


char *Str_cat(const char *s, ...) {
	char *t = NULL;
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
		int size = STRLEN;
		buf = ALLOC(size);
		while (true) {
                        va_list ap_copy;
                        va_copy(ap_copy, ap);
			n = vsnprintf(buf, size, s, ap_copy);
                        va_end(ap_copy);
			if (n > -1 && n < size)
				break;
			if (n > -1)
				size = n+1;
			else
				size*= 2;
			RESIZE(buf, size);
		}
	}
	return buf;
}


int Str_parseInt(const char *s) {
	int i;
        char *e;
	if (! (s && *s))
		THROW(SQLException, "For input string null");
        errno = 0;
	i = (int)strtol(s, &e, 10);
	if (errno || (e == s))
		THROW(SQLException, "For input string %s -- %s", s, STRERROR);
	return i;
}


long long int Str_parseLLong(const char *s) {
        char *e;
	long long l;
	if (! (s && *s))
		THROW(SQLException, "For input string null");
        errno = 0;
	l = strtoll(s, &e, 10);
	if (errno || (e == s))
		THROW(SQLException, "For input string %s -- %s", s, STRERROR);
	return l;
}


double Str_parseDouble(const char *s) {
        char *e;
	double d;
	if (! (s && *s))
		THROW(SQLException, "For input string null");
        errno = 0;
	d = strtod(s, &e);
	if (errno || (e == s))
		THROW(SQLException, "For input string %s -- %s", s, STRERROR);
	return d;
}


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif

