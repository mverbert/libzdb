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


#ifndef MEM_INCLUDED
#define MEM_INCLUDED


/**
 * General purpose memory allocation <b>Class methods</b>.
 *
 * @file
 */


/**
 * Allocate <code>n</code> bytes of memory.
 * @param n number of bytes to allocate
 * @return A pointer to the newly allocated memory
 * @exception MemoryException if allocation failed
 * @exception AssertException if <code>n <= 0</code> 
 * @hideinitializer
 */
#define ALLOC(n) Mem_alloc((n), __func__, __FILE__, __LINE__)


/**
 * Allocate <code>c</code> objects of size <code>n</code> each.
 * Same as calling ALLOC(c * n) except this function also clear
 * the memory region before it is returned. 
 * @param c number of objects to allocate
 * @param n object size in bytes
 * @return A pointer to the newly allocated memory
 * @exception MemoryException if allocation failed
 * @exception AssertException if <code>c or n <= 0</code> 
 * @hideinitializer
 */
#define CALLOC(c, n) Mem_calloc((c), (n), __func__, __FILE__, __LINE__)


/**
 * Allocate <code>p</code> and clear the memory region
 * before the allocated object is returned. 
 * @param p ADT object to allocate
 * @exception MemoryException if allocation failed
 * @hideinitializer
 */
#define NEW(p) ((p) = CALLOC(1, (long)sizeof *(p)))


/**
 * Deallocates <code>p</code>
 * @param p object to deallocate
 * @hideinitializer
 */
#define FREE(p) ((void)(Mem_free((p), __func__, __FILE__, __LINE__), (p) = 0))


/**
 * Reallocate <code>p</code> with size <code>n</code>.
 * @param p pointer to reallocate
 * @param n new object size in bytes
 * @exception MemoryException if allocation failed
 * @exception AssertException if <code>n <= 0</code> 
 * @hideinitializer
 */
#define RESIZE(p, n) ((p)= Mem_resize((p), (n), __func__, __FILE__, __LINE__))


/**
 * Allocate and return <code>size</code> bytes of memory. If 
 * allocation failed this method throws AssertException
 * @param size The number of bytes to allocate
 * @param func caller
 * @param file location of caller
 * @param line location of caller
 * @exception MemoryException if allocation failed
 * @exception AssertException if <code>n <= 0</code> 
 * @return a pointer to the allocated memory
 */
void *Mem_alloc(long size, const char *func, const char *file, int line);


/**
 * Allocate and return memory for <code>count</code> objects, each of 
 * <code>size</code> bytes. The returned memory is cleared. If allocation
 * failed this method throws AssertException
 * @param count The number of objects to allocate
 * @param size The size of each object to allocate
 * @param func caller
 * @param file location of caller
 * @param line location of caller
 * @exception MemoryException if allocation failed
 * @exception AssertException if <code>c or n <= 0</code> 
 * @return a pointer to the allocated memory 
 */
void *Mem_calloc(long count, long size, const char *func, const char *file, int line);


/**
 * Deallocate the memory pointed to by <code>p</code>
 * @param p The memory to deallocate
 * @param func caller
 * @param file location of caller
 * @param line location of caller
 */
void Mem_free(void *p, const char *func, const char *file, int line);


/**
 * Resize the allocation pointed to by <code>p</code> by <code>size</code>
 * bytes and return the changed allocation. If allocation failed this 
 * method throws AssertException
 * @param p A pointer to the allocation to change
 * @param size The new size of <code>p</code>
 * @param func caller
 * @param file location of caller
 * @param line location of caller
 * @exception MemoryException if allocation failed
 * @exception AssertException if <code>n <= 0</code> 
 * @return a pointer to the changed memory 
 */
void *Mem_resize(void *p, long size, const char *func, const char *file, int line);


#endif
