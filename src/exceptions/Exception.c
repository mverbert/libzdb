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

/*
 * Copyright (c) 1994,1995,1996,1997 by David R. Hanson.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 * 
 * <http://www.opensource.org/licenses/mit-license.php>
 */


#include "Config.h"

#include <stdio.h>
#include <string.h>

#include "Thread.h"
#include "Exception.h"


/**
 * Implementation of the Exception interface. Defines the Thread local 
 * Exception stack and Exceptions used in the library. 
 * 
 * LEGAL NOTICE, the part that differs from the original code is Copyright
 * (C) Tildeslash Ltd. This implementation is licensed under GPLv3 with 
 * Exceptions in the file EXCEPTIONS. The MIT license above applies to the
 * original and exceptional clever CII code which can be found at this URL
 * http://www.cs.princeton.edu/software/cii/
 *
 * @version \$Id: Exception.c,v 1.28 2008/03/20 11:28:54 hauk Exp $
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#define T Exception_T
/* Placeholder for system exceptions. */
T SQLException = {"SQLException"};
#ifdef ZILD_PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif
T AssertException = {"AssertException"};
/* Thread specific Exception stack */
ThreadData_T Exception_stack;
#ifdef ZILD_PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif
static pthread_once_t once_control = PTHREAD_ONCE_INIT;


/* -------------------------------------------------------- Privat methods */


static void init_once() { ThreadData_create(Exception_stack); }


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif


void Exception_init() {
        pthread_once(&once_control, init_once);
}


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif


/* -------------------------------------------------------- Public methods */

#ifndef ZILD_PACKAGE_PROTECTED

void Exception_throw(const T *e, const char *func, const char *file, int line, const char *cause, ...) {
        char message[EXCEPTION_MESSAGE_LENGTH + 1] = "?";
	Exception_Frame *p = ThreadData_get(Exception_stack);
	assert(e);
	assert(e->name);
        if (cause) {
                va_list ap;
                va_start(ap, cause);
                vsnprintf(message, EXCEPTION_MESSAGE_LENGTH, cause, ap);
                va_end(ap);
        }
	if (p == NULL) {
                ABORT("%s%s%s\n raised in %s at %s:%d\n", e->name, cause ? ": " : "", cause ? message : "", func ? func : "?", file ? file : "?", line);
	} else {
                p->exception = e;
                p->func = func;
                p->file = file;
                p->line = line;
                if (cause)
                        Str_copy(p->message, message, EXCEPTION_MESSAGE_LENGTH);
                pop_exception_stack;	
                longjmp(p->env, Exception_throwd);
        }
}

#endif
