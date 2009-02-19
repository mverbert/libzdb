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


#ifndef PREPAREDSTATEMENT_INCLUDED
#define PREPAREDSTATEMENT_INCLUDED
//<< Start filter-out
#include "PreparedStatementStrategy.h"
//>> End filter-out


/**
 * A <b>PreparedStatement</b> represent a single SQL statement pre-compiled 
 * into byte code for later execution. The SQL statement may contain 
 * <b>IN</b> parameters of the form "?". Such parameters represent 
 * unspecified literal values (or "wildcards") to be filled in later by the 
 * various setter methods defined in this interface. Each IN parameter has an
 * associated index number which is its sequence in the statement. The first 
 * IN '?' parameter has index 1, the next has index 2 and so on. A 
 * PreparedStatement is created by the Connection_prepareStatement() method.
 * 
 * Consider this statement: 
 * <pre>
 *  INSERT INTO employee(name, picture) VALUES(?, ?)
 * </pre>
 * There are two IN parameters in this statement, the parameter for setting
 * the name has index 1 and the one for the picture has index 2. To set the 
 * values for the IN parameters we use a setter method. Assuming name has
 * a string value we use PreparedStatement_setString(). To set the value
 * of the picture we submit a binary value using the 
 * method PreparedStatement_setBlob(). 
 *
 * Please note that string and blob parameter values are set by reference 
 * and <b>must</b> not "disappear" before either PreparedStatement_execute()
 * or PreparedStatement_executeQuery() is called. 
 * 
 * <h2>Example:</h2>
 * To summarize, here is the code in context. 
 * <pre>
 * PreparedStatement_T p = Connection_prepareStatement(con, "INSERT INTO employee(name, picture) VALUES(?, ?)");
 * PreparedStatement_setString(p, 1, "Kamiya Kaoru");
 * PreparedStatement_setBlob(p, 2, jpeg, size);
 * PreparedStatement_execute(p);
 * </pre>
 * <h2>Reuse and ResultSets:</h2>
 * A PreparedStatement can be reused. That is, the method 
 * PreparedStatement_execute() can be called one or more times to execute 
 * the same statement. Clients can also set new IN parameter values and
 * re-execute the statement as shown in this example:
 * <pre>
 * PreparedStatement_T p = Connection_prepareStatement(con, "INSERT INTO employee(name, picture) VALUES(?, ?)");
 * for (int i = 0; employees[i].name; i++) 
 * {
 *        PreparedStatement_setString(p, 1, employees[i].name);
 *        PreparedStatement_setBlob(p, 2, employees[i].picture, employees[i].picture_size);
 *        PreparedStatement_execute(p);
 * }
 * </pre>
 * <b>Note,</b> a ResultSet returned from PreparedStatement_executeQuery()  
 * "lives" only until the next call to a PreparedStatement execute method
 * or until the Connection is returned to the Connection Pool. 
 * 
 * @see Connection.h ResultSet.h SQLException.h
 * @file
 */


#define T PreparedStatement_T
typedef struct T *T;

//<< Start filter-out

/**
 * Create a new PreparedStatement.
 * @param I the implementation used by this PreparedStatement
 * @param op implementation opcodes
 * @return A new PreparedStatement object
 */
T PreparedStatement_new(PreparedStatementImpl_T I, Pop_T op);


/**
 * Destroy a PreparedStatement and release allocated resources.
 * @param P A PreparedStatement object reference
 */
void PreparedStatement_free(T *P);

//>> End filter-out

/**
 * Sets the IN parameter at index <code>parameterIndex</code> to the 
 * given string value. 
 * @param P A PreparedStatement object
 * @param parameterIndex the first parameter is 1, the second is 2,..
 * @param x The string value to set. Must be a NUL terminated string.
 * @exception SQLException if a database access error occurs or if parameter 
 * index is out of range
 * @see SQLException.h
*/
void PreparedStatement_setString(T P, int parameterIndex, const char *x);


/**
 * Sets the IN parameter at index <code>parameterIndex</code> to the 
 * given int value. 
 * @param P A PreparedStatement object
 * @param parameterIndex the first parameter is 1, the second is 2,..
 * @param x The int value to set
 * @exception SQLException if a database access error occurs or if parameter 
 * index is out of range
 * @see SQLException.h
 */
void PreparedStatement_setInt(T P, int parameterIndex, int x);


/**
 * Sets the IN parameter at index <code>parameterIndex</code> to the 
 * given long long value. 
 * @param P A PreparedStatement object
 * @param parameterIndex the first parameter is 1, the second is 2,..
 * @param x The long long value to set
 * @exception SQLException if a database access error occurs or if parameter 
 * index is out of range
 * @see SQLException.h
 */
void PreparedStatement_setLLong(T P, int parameterIndex, long long int x);


/**
 * Sets the IN parameter at index <code>parameterIndex</code> to the 
 * given double value. 
 * @param P A PreparedStatement object
 * @param parameterIndex the first parameter is 1, the second is 2,..
 * @param x The double value to set
 * @exception SQLException if a database access error occurs or if parameter 
 * index is out of range
 * @see SQLException.h
 */
void PreparedStatement_setDouble(T P, int parameterIndex, double x);


/**
 * Sets the IN parameter at index <code>parameterIndex</code> to the 
 * given blob value. 
 * @param P A PreparedStatement object
 * @param parameterIndex the first parameter is 1, the second is 2,..
 * @param x The blob value to set
 * @param size the number of bytes in the blob 
 * @exception SQLException if a database access error occurs or if parameter 
 * index is out of range
 * @see SQLException.h
 */
void PreparedStatement_setBlob(T P, int parameterIndex, const void *x, int size);


/**
 * Executes the prepared SQL statement, which may be an INSERT, UPDATE,
 * or DELETE statement or an SQL statement that returns nothing, such
 * as an SQL DDL statement. 
 * @param P A PreparedStatement object
 * Connection_getLastError() can be used to obtain the error string.
 * @exception SQLException if a database error occurs
 * @see SQLException.h
 */
void PreparedStatement_execute(T P);


/**
 * Executes the prepared SQL statement, which returns a single ResultSet
 * object. A ResultSet "lives" only until the next call to a PreparedStatement 
 * method or until the Connection is returned to the Connection Pool. 
 * <i>This means that Result Sets cannot be saved between queries</i>.
 * @param P A PreparedStatement object
 * @return A ResultSet object that contains the data produced by the
 * prepared statement. If there was an error NULL is returned and the method
 * Connection_getLastError() can be used to obtain the error string.
 * @exception SQLException if a database error occurs
 * @see ResultSet.h
 * @see SQLException.h
 */
ResultSet_T PreparedStatement_executeQuery(T P);


#undef T
#endif
