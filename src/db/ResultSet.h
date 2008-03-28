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


#ifndef RESULTSET_INCLUDED
#define RESULTSET_INCLUDED
//<< Start filter-out
#include "ResultSetStrategy.h"
//>> End filter-out


/**
 * A <b>ResultSet</b> represents a database result set. A ResultSet is
 * created by executing a SQL SELECT statement using the
 * Connection_executeQuery() method or by using a prepared statement's
 * PreparedStatement_executeQuery() method.
 *
 * A ResultSet object maintains a cursor pointing to its current row
 * of data. Initially the cursor is positioned before the first
 * row. The ResultSet_next() method moves the cursor to the next row,
 * and because it returns false when there are no more rows in the
 * ResultSet object, it can be used in a while loop to iterate through
 * the result set.  A ResultSet object is not updatable and has a
 * cursor that moves forward only. Thus, you can iterate through it
 * only once and only from the first row to the last row.
 *
 * The ResultSet interface provides getter methods for retrieving
 * column values from the current row. Values can be retrieved using
 * either the index number of the column or the name of the column. In
 * general, using the column index will be more efficient. <i>Columns
 * are numbered from 1.</i> 
 *
 * Column names used as input to getter methods are case sensitive.
 * When a getter method is called with a column name and several
 * columns have the same name, the value of the first matching column
 * will be returned. The column name option is designed to be used
 * when column names are used in the SQL query that generated the
 * result set. For columns that are NOT explicitly named in the query,
 * it is best to use column indices. If column names are used, there
 * is no way for the programmer to guarantee that they actually refer
 * to the intended columns.
 *
 * <h3>Example</h3>
 * The following examples demonstrate how to obtain a ResultSet and get 
 * values from the set:
 * <pre>
 * ResultSet_T r = Connection_executeQuery(con, "SELECT ssn, name, picture FROM CUSTOMERS");
 * while (ResultSet_next(r)) 
 * {
 *      int ssn = ResultSet_getIntByName(r, "ssn");
 *      const char *name =  ResultSet_getStringByName(r, "name");
 *      int blobSize;
 *      void *picture = ResultSet_getBlobByName(r, "picture", &blobSize);
 *      [..]
 * }
 * </pre>
 * Here is another example where a generated result is selected and printed:
 * <pre>
 * ResultSet_T r = Connection_executeQuery(con, "SELECT count(*) FROM USERS");
 * if (ResultSet_next(r))
 *  	printf("Number of users: %d\n", ResultSet_getInt(r, 1));
 * </pre>
 *
 * @see Connection.h PreparedStatement.h SQLException.h
 * @version \$Id: ResultSet.h,v 1.28 2008/02/23 19:57:28 hauk Exp $
 * @file
 */


#define T ResultSet_T
typedef struct T *T;

//<< Start filter-out

/**
 * Create a new ResultSet.
 * @param I the implementation used by this ResultSet
 * @param op implementation opcodes
 * @return A new ResultSet object
 */
T ResultSet_new(IResultSet_T I, Rop_T op);


/**
 * Destroy a ResultSet and release allocated resources.
 * @param R A ResultSet object reference
 */
void ResultSet_free(T *R);

//>> End filter-out

/** @name Properties */
//@{

/**
 * Returns the number of columns in this ResultSet object.
 * @param R A ResultSet object
 * @return The number of columns
 */
int ResultSet_getColumnCount(T R);


/**
 * Get the designated column's name.
 * @param R A ResultSet object
 * @param column the first column is 1, the second is 2, and so on
 * @return Column name or NULL if the column does not exist. You 
 * should use the method ResultSet_getColumnCount() to test for 
 * the availablity of columns in the result set.
 */
const char *ResultSet_getColumnName(T R, int column);


/**
 * Returns column size in bytes. If the column is a blob then 
 * this methtod returns the number of bytes in that blob. No type 
 * conversions occur. If the result is a string (or a number 
 * since a number can be converted into a string) then return the 
 * number of bytes in the resulting string. If <code>columnIndex</code>
 * is outside the range [1..ResultSet_getColumnCount()] this
 * method returns -1.
 * @param R A ResultSet object
 * @param columnIndex the first column is 1, the second is 2, ...
 * @return column data size
 * @exception SQLException if columnIndex is outside the valid range
 * @see SQLException.h
 */
long ResultSet_getColumnSize(T R, int columnIndex);

//@}

/**
 * Moves the cursor down one row from its current position. A
 * ResultSet cursor is initially positioned before the first row; the
 * first call to this method makes the first row the current row; the
 * second call makes the second row the current row, and so on. When
 * there are not more available rows false is returned. An empty
 * ResultSet will return false on the first call to ResultSet_next().
 * @param R A ResultSet object
 * @return true if the new current row is valid; false if there are no
 * more rows
 */
int ResultSet_next(T R);


/**
 * Retrieves the value of the designated column in the current row of
 * this ResultSet object as a C-string. If <code>columnIndex</code>
 * is outside the range [1..ResultSet_getColumnCount()] this
 * method returns NULL. <i>The returned string may only be valid until 
 * the next call to ResultSet_next() and if you plan to use the returned 
 * value longer, you must make a copy.</i>
 * @param R A ResultSet object
 * @param columnIndex the first column is 1, the second is 2, ...
 * @return the column value; if the value is SQL NULL, the value
 * returned is NULL
 * @exception SQLException if a database access error occurs or 
 * columnIndex is outside the valid range
 * @see SQLException.h
 */
const char *ResultSet_getString(T R, int columnIndex);


/**
 * Retrieves the value of the designated column in the current row of
 * this ResultSet object as a C-string. If <code>columnName</code>
 * is not found this method returns NULL. <i>The returned string may 
 * only be valid until the next call to ResultSet_next() and if you plan
 * to use the returned value longer, you must make a copy.</i>
 * @param R A ResultSet object
 * @param columnName the SQL name of the column. <i>case-sensitive</i>
 * @return the column value; if the value is SQL NULL, the value
 * returned is NULL
 * @exception SQLException if a database access error occurs or 
 * columnName does not exist
 * @see SQLException.h
 */
const char *ResultSet_getStringByName(T R, const char *columnName);


/**
 * Retrieves the value of the designated column in the current row of
 * this ResultSet object as an int. If <code>columnIndex</code>
 * is outside the range [1..ResultSet_getColumnCount()] this
 * method returns 0.
 * @param R A ResultSet object
 * @param columnIndex the first column is 1, the second is 2, ...
 * @return the column value; if the value is SQL NULL, the value
 * returned is 0
 * @exception SQLException if a database access error occurs or 
 * columnIndex is outside the valid range
 * @see SQLException.h
 */
int ResultSet_getInt(T R, int columnIndex);


/**
 * Retrieves the value of the designated column in the current row of
 * this ResultSet object as an int. If <code>columnName</code> is not 
 * found this method returns 0.
 * @param R A ResultSet object
 * @param columnName the SQL name of the column. <i>case-sensitive</i>
 * @return the column value; if the value is SQL NULL, the value
 * returned is 0
 * @exception SQLException if a database access error occurs or 
 * columnName does not exist
 * @see SQLException.h
 */
int ResultSet_getIntByName(T R, const char *columnName);


/**
 * Retrieves the value of the designated column in the current row of
 * this ResultSet object as a long long. If <code>columnIndex</code>
 * is outside the range [1..ResultSet_getColumnCount()] this
 * method returns 0.
 * @param R A ResultSet object
 * @param columnIndex the first column is 1, the second is 2, ...
 * @return the column value; if the value is SQL NULL, the value
 * returned is 0
 * @exception SQLException if a database access error occurs or 
 * columnIndex is outside the valid range
 * @see SQLException.h
 */
long long int ResultSet_getLLong(T R, int columnIndex);


/**
 * Retrieves the value of the designated column in the current row of
 * this ResultSet object as a long long. If <code>columnName</code>
 * is not found this method returns 0.
 * @param R A ResultSet object
 * @param columnName the SQL name of the column. <i>case-sensitive</i>
 * @return the column value; if the value is SQL NULL, the value
 * returned is 0
 * @exception SQLException if a database access error occurs or 
 * columnName does not exist
 * @see SQLException.h
 */
long long int ResultSet_getLLongByName(T R, const char *columnName);


/**
 * Retrieves the value of the designated column in the current row of
 * this ResultSet object as a double. If <code>columnIndex</code>
 * is outside the range [1..ResultSet_getColumnCount()] this
 * method returns 0.0.
 * @param R A ResultSet object
 * @param columnIndex the first column is 1, the second is 2, ...
 * @return the column value; if the value is SQL NULL, the value
 * returned is 0.0
 * @exception SQLException if a database access error occurs or 
 * columnIndex is outside the valid range
 * @see SQLException.h
 */
double ResultSet_getDouble(T R, int columnIndex);


/**
 * Retrieves the value of the designated column in the current row of
 * this ResultSet object as a double. If <code>columnName</code> is 
 * not found this method returns 0.0.
 * @param R A ResultSet object
 * @param columnName the SQL name of the column. <i>case-sensitive</i>
 * @return the column value; if the value is SQL NULL, the value
 * returned is 0.0
 * @exception SQLException if a database access error occurs or 
 * columnName does not exist
 * @see SQLException.h
 */
double ResultSet_getDoubleByName(T R, const char *columnName);


/**
 * Retrieves the value of the designated column in the current row of
 * this ResultSet object as a void pointer. If <code>columnIndex</code>
 * is outside the range [1..ResultSet_getColumnCount()] this method 
 * returns NULL. This method allocate <code>size</code> bytes of memory 
 * for the returned blob. If the size of the blob is expected to be 
 * "large", consider instead using the method ResultSet_readData() 
 * defined below and read the content of the blob in chunks. <i>The 
 * returned blob may only be valid until the next call to ResultSet_next()
 * and if you plan to use the returned value longer, you must make a copy.</i> 
 * @param R A ResultSet object
 * @param columnIndex the first column is 1, the second is 2, ...
 * @param size the number of bytes in the blob is stored in size 
 * @return the column value; if the value is SQL NULL, the value
 * returned is NULL
 * @exception SQLException if a database access error occurs or 
 * columnIndex is outside the valid range
 * @see SQLException.h
 */
const void *ResultSet_getBlob(T R, int columnIndex, int *size);


/**
 * Retrieves the value of the designated column in the current row of
 * this ResultSet object as a void pointer. If <code>columnName</code>
 * is not found this method returns NULL. This method allocate 
 * <code>size</code> bytes of memory for the returned blob. If the size
 * of the blob is expected to be "large", consider instead using the 
 * method ResultSet_readData() defined below and read the content of the
 * blob in chunks. <i>The returned blob may only be valid until the next 
 * call to ResultSet_next() and if you plan to use the returned value 
 * longer, you must make a copy.</i>
 * @param R A ResultSet object
 * @param columnName the SQL name of the column. <i>case-sensitive</i>
 * @param size the number of bytes in the blob is stored in size 
 * @return the column value; if the value is SQL NULL, the value
 * returned is NULL
 * @exception SQLException if a database access error occurs or 
 * columnName does not exist
 * @see SQLException.h
 */
const void *ResultSet_getBlobByName(T R, const char *columnName, int *size);


/**
 * Reads <code>length</code> bytes from a blob or text field and stores 
 * them into the byte buffer pointed to by <code>b</code>. Reading stops 
 * when <code>length</code> bytes are read or less. The buffer, 
 * <code>b</code>, is <b>not</b> NUL terminated. Example on use;
 * <pre>
 * r = Connection_executeQuery(con, "select text from books;");
 * while (ResultSet_next(r)) {
 *        int n = 0;
 *        long off = 0;
 *        \#define BUFSIZE 8192
 *        unsigned char buf[BUFSIZE + 1];
 *        while ((n = ResultSet_readData(r, 1, buf, BUFSIZE, off)) > 0) {
 *                buf[n] = 0;
 *                puts(buf);
 *                off+= n;
 *        }
 * }
 *</pre>
 * It is a checked runtime error for <code>b</code> to be NULL
 * @param R A ResultSet object
 * @param columnIndex the first column is 1, the second is 2, ...
 * @param b A byte buffer
 * @param length The size of the buffer b
 * @param off The offset to start reading data from
 * @return Number of bytes read or -1 if an error occured. 0 is returned
 * when end of data was reached
 * @exception SQLException if a database access error occurs or 
 * columnIndex is outside the valid range
 * @see SQLException.h
 */
int ResultSet_readData(T R, int columnIndex, void *b, int length, long off);


#undef T
#endif
