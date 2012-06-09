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


#ifndef STRINGBUFFER_INCLUDED
#define STRINGBUFFER_INCLUDED
#include <stdarg.h>


/** 
 * A <b>String Buffer</b> implements a mutable sequence of characters.
 *
 * @file
 */


#define T StringBuffer_T
typedef struct T *T;


/**
 * Constructs a string buffer so that it represents the same sequence of 
 * characters as the string argument; in other  words, the initial contents 
 * of the string buffer is a copy of the argument string. 
 * @param s the initial contents of the buffer
 * @return A new StringBuffer object
 */
T StringBuffer_new(const char *s);


/**
 * Factory method, create an empty string buffer
 * @param hint The initial capacity of the buffer in bytes (hint > 0)
 * @return A new StringBuffer object
 * @exception AssertException if hint is less than or equal to 0
 * @exception MemoryException if allocation failed
 */
T StringBuffer_create(int hint);


/**
 * Destroy a StringBuffer object and free allocated resources
 * @param S a StringBuffer object reference
 */
void StringBuffer_free(T *S);


/**
 * The characters of the String argument are appended, in order, to the 
 * contents of this string buffer, increasing the length of this string 
 * buffer by the length of the arguments. 
 * @param S StringBuffer object
 * @param s A string with optional var args
 * @return A reference to this StringBuffer
 */
T StringBuffer_append(T S, const char *s, ...) __attribute__((format (printf, 2, 3)));


/**
 * The characters of the String argument are appended, in order, to the 
 * contents of this string buffer, increasing the length of this string 
 * buffer by the length of the arguments. 
 * @param S StringBuffer object
 * @param s A string with optional var args
 * @param ap A variable argument list
 * @return A reference to this StringBuffer
 */
T StringBuffer_vappend(T S, const char *s, va_list ap);


/**
 * Returns the length (character count) of this string buffer.
 * @param S StringBuffer object
 * @return The length of the sequence of characters currently represented 
 * by this string buffer
 */
int StringBuffer_length(T S);


/**
 * Clear the contents of the string buffer. I.e. set buffer length to 0.
 * @param S StringBuffer object
 * @return a reference to this StringBuffer
 */
T StringBuffer_clear(T S);


/**
 * Converts to a string representing the data in this string buffer.
 * @param S StringBuffer object
 * @return A string representation of the string buffer 
 */
const char *StringBuffer_toString(T S);


/**
 * Replace all occurences of <code>?</code> in this string buffer with <code>$n</code>.
 * Example: 
 * <pre>
 * StringBuffer_T b = StringBuffer_new("insert into host values(?, ?, ?);"); 
 * StringBuffer_prepare4postgres(b) -> "insert into host values($1, $2, $3);"
 * </pre>
 * @param S StringBuffer object
 * @return The number of replacements that took place
 * @exception SQLException if there are more than 99 wild card '?' parameters
 */
int StringBuffer_prepare4postgres(T S);


/**
 * Replace all occurences of <code>?</code> in this string buffer with <code>:n</code>.
 * Example: 
 * <pre>
 * StringBuffer_T b = StringBuffer_new("insert into host values(?, ?, ?);"); 
 * StringBuffer_prepare4oracle(b) -> "insert into host values(:1, :2, :3);"
 * </pre>
 * @param S StringBuffer object
 * @return The number of replacements that took place
 * @exception SQLException if there are more than 99 wild card '?' parameters
 */
int StringBuffer_prepare4oracle(T S);


/**
 * Remove (any) leading and trailing white space and semicolon [ \\t\\r\\n;]. Example
 * <pre>
 * StringBuffer_T b = StringBuffer_new("\t select a from b; \n"); 
 * StringBuffer_trim(b) -> "select a from b"
 * </pre>
 * @param S StringBuffer object
 * @return a reference to this StringBuffer
 */
T StringBuffer_trim(T S);


#undef T
#endif
