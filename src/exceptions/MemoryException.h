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


#ifndef MEMORYEXCEPTION_INCLUDED
#define MEMORYEXCEPTION_INCLUDED
#include <Exception.h>


/**
 * Thrown to indicate that a memory allocation failed. Every object 
 * constructor method may throw a MemoryException if the underlying
 * allocator failed.
 * @see Exception.h, Mem.h
 * @file
 */
extern Exception_T MemoryException;


#endif
