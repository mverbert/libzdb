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

#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "assert.h"


/**
 * Implementation of the Mem interface
 *
 * @version \$Id: Mem.c,v 1.1 2008/03/20 11:28:54 hauk Exp $
 * @file
 */


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif


void *Mem_alloc(long size, const char *file, int line){
	void *p;
	assert(size > 0);
	p = malloc(size);
	if (p == NULL)
		THROW(AssertException, "MemoryException -- %s at %s in line %d\n", STRERROR, file, line);
	return p;
}


void *Mem_calloc(long count, long size, const char *file, int line) {
	void *p;
	assert(count > 0);
	assert(size > 0);
	p = calloc(count, size);
	if (p == NULL)
		THROW(AssertException, "MemoryException -- %s at %s in line %d\n", STRERROR, file, line);
	return p;
}


void Mem_free(void *p, const char *file, int line) {
	if (p)
		free(p);
}


void *Mem_resize(void *p, long size, const char *file, int line) {
	assert(p);
	assert(size > 0);
	p = realloc(p, size);
	if (p == NULL)
		THROW(AssertException, "MemoryException -- %s at %s in line %d\n", STRERROR, file, line);
	return p;
}


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif


