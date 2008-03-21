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

#include "StringBuffer.h"


/**
 * Implementation of the StringBuffer interface.
 *
 * @version \$Id: StringBuffer.c,v 1.24 2008/03/20 11:28:54 hauk Exp $
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#define BUFFER_SIZE 256

#define T StringBuffer_T
struct T {
        int used;
        int length;
	uchar_t *buffer;
};


/* ------------------------------------------------------- Private methods */


static inline void doAppend(T S, const char *s, va_list ap) {
        while (true) {
                int n = vsnprintf(S->buffer + S->used, S->length-S->used, s, ap);
                if (n > -1 && (S->used + n) < S->length) {
                        S->used+= n;
                        break;
                }
                if (n > -1)
                        S->length+= BUFFER_SIZE + n;
                else
                        S->length *= 2;
                RESIZE(S->buffer, S->length);
        }
}


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif


T StringBuffer_new(const char *s) {
        T S;
        NEW(S);
        S->used = 0;
        S->length = BUFFER_SIZE;
        S->buffer = ALLOC(BUFFER_SIZE);
        return StringBuffer_append(S, s);
}


void StringBuffer_free(T *S) {
        assert(S && *S);
	FREE((*S)->buffer);
        FREE(*S);
}


T StringBuffer_append(T S, const char *s, ...) {
        assert(S);
        if (s && *s) {
                va_list ap;
                va_start(ap, s);
                doAppend(S, s, ap);
                va_end(ap);
        }
        return S;
}


T StringBuffer_vappend(T S, const char *s, va_list ap) {
        assert(S);
        if (s && *s)
                doAppend(S, s, ap);
        return S;
}


int StringBuffer_length(T S) {
        assert(S);
        return S->used;
}


void StringBuffer_clear(T S) {
        assert(S);
        S->used = 0;
        *S->buffer = 0;
}


const char *StringBuffer_toString(T S) {
        assert(S);
        return S->buffer;
}


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif

