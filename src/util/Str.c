/*
 * Copyright (C) 2004-2010 Tildeslash Ltd. All rights reserved.
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


// We don't use strdup to support MemoryException on OOM
char *Str_dup(const char *s) { 
        char *t = NULL;
        if (s) {
                int n = strlen(s); 
                t = ALLOC(n + 1);
                memcpy(t, s, n);
                t[n]= 0;
        }
        return t;
}


char *Str_ndup(const char *s, int n) {
        char *t = NULL;
        assert(n >= 0);
        if (s) {
                int l = strlen(s); 
                n = l < n ? l : n; // Use the actual length of s if shorter than n
                t = ALLOC(n + 1);
                memcpy(t, s, n);
                t[n]= 0;
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
	char *t = 0;
	if (s) {
                int n = 0;
		int size = STRLEN;
		t = ALLOC(size);
		while (true) {
                        va_list ap_copy;
                        va_copy(ap_copy, ap);
			n = vsnprintf(t, size, s, ap_copy);
                        va_end(ap_copy);
			if (n > -1 && n < size)
				break;
			if (n > -1)
				size = n+1;
			else
				size*= 2;
			RESIZE(t, size);
		}
	}
	return t;
}


int Str_parseInt(const char *s) {
	int i;
        char *e;
	if (! (s && *s))
		THROW(SQLException, "NumberFormatException: For input string null");
        errno = 0;
	i = (int)strtol(s, &e, 10);
	if (errno || (e == s))
		THROW(SQLException, "NumberFormatException: For input string %s -- %s", s, STRERROR);
	return i;
}


long long int Str_parseLLong(const char *s) {
        char *e;
	long long l;
	if (! (s && *s))
		THROW(SQLException, "NumberFormatException: For input string null");
        errno = 0;
	l = strtoll(s, &e, 10);
	if (errno || (e == s))
		THROW(SQLException, "NumberFormatException: For input string %s -- %s", s, STRERROR);
	return l;
}


double Str_parseDouble(const char *s) {
        char *e;
	double d;
	if (! (s && *s))
		THROW(SQLException, "NumberFormatException: For input string null");
        errno = 0;
	d = strtod(s, &e);
	if (errno || (e == s))
		THROW(SQLException, "NumberFormatException: For input string %s -- %s", s, STRERROR);
	return d;
}

#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif

