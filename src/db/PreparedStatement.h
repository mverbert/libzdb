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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.
 */


#ifndef PREPAREDSTATEMENT_INCLUDED
#define PREPAREDSTATEMENT_INCLUDED
//<< Protected methods
#include "PreparedStatementDelegate.h"
//>> End Protected methods


/**
 * A <b>PreparedStatement</b> represent a single SQL statement pre-compiled 
 * into byte code for later execution. The SQL statement may contain 
 * <i>in</i> parameters of the form "?". Such parameters represent 
 * unspecified literal values (or "wildcards") to be filled in later by the 
 * various setter methods defined in this interface. Each <i>in</i> parameter has an
 * associated index number which is its sequence in the statement. The first 
 * <i>in</i> '?' parameter has index 1, the next has index 2 and so on. A 
 * PreparedStatement is created by calling Connection_prepareStatement().
 * 
 * Consider this statement: 
 * <pre>
 *  INSERT INTO employee(name, picture) VALUES(?, ?)
 * </pre>
 * There are two <i>in</i> parameters in this statement, the parameter for setting
 * the name has index 1 and the one for the picture has index 2. To set the 
 * values for the <i>in</i> parameters we use a setter method. Assuming name has
 * a string value we use PreparedStatement_setString(). To set the value
 * of the picture we submit a binary value using the 
 * method PreparedStatement_setBlob(). 
 *
 * Note that string and blob parameter values are set by reference and 
 * <b>must</b> not "disappear" before either PreparedStatement_execute()
 * or PreparedStatement_executeQuery() is called. 
 * 
 * <h3>Example:</h3>
 * To summarize, here is the code in context. 
 * <pre>
 * PreparedStatement_T p = Connection_prepareStatement(con, "INSERT INTO employee(name, picture) VALUES(?, ?)");
 * PreparedStatement_setString(p, 1, "Kamiya Kaoru");
 * PreparedStatement_setBlob(p, 2, jpeg, jpeg_size);
 * PreparedStatement_execute(p);
 * </pre>
 * <h3>Reuse:</h3>
 * A PreparedStatement can be reused. That is, the method 
 * PreparedStatement_execute() can be called one or more times to execute 
 * the same statement. Clients can also set new <i>in</i> parameter values and
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
 * <h3>Result Sets:</h3>
 * Here is another example where we use a Prepared Statement to execute a query
 * which returns a Result Set:
 * 
 * <pre>
 * PreparedStatement_T p = Connection_prepareStatement(con, "SELECT id FROM employee WHERE name LIKE ?"); 
 * PreparedStatement_setString(p, 1, "%Kaoru%");
 * ResultSet_T r = PreparedStatement_executeQuery(p);
 * while (ResultSet_next(r))
 *        printf("employee.id = %d\n", ResultSet_getInt(r, 1));
 * </pre>
 * 
 * A ResultSet returned from PreparedStatement_executeQuery() "lives" until
 * the Prepared Statement is executed again or until the Connection is
 * returned to the Connection Pool. 
 *
 * <i>A PreparedStatement is reentrant, but not thread-safe and should only be used by one thread (at the time).</i>
 * 
 * @see Connection.h ResultSet.h SQLException.h
 * @file
 */


#define T PreparedStatement_T
typedef struct PreparedStatement_S *T;

//<< Protected methods

/**
 * Create a new PreparedStatement.
 * @param D the delegate used by this PreparedStatement
 * @param op delegate operations
 * @return A new PreparedStatement object
 */
T PreparedStatement_new(PreparedStatementDelegate_T D, Pop_T op);


/**
 * Destroy a PreparedStatement and release allocated resources.
 * @param P A PreparedStatement object reference
 */
void PreparedStatement_free(T *P);

//>> End Protected methods

/**
 * Sets the <i>in</i> parameter at index <code>parameterIndex</code> to the 
 * given string value. 
 * @param P A PreparedStatement object
 * @param parameterIndex The first parameter is 1, the second is 2,..
 * @param x The string value to set. Must be a NUL terminated string. NULL
 * is allowed to indicate a SQL NULL value. 
 * @exception SQLException if a database access error occurs or if parameter 
 * index is out of range
 * @see SQLException.h
*/
void PreparedStatement_setString(T P, int parameterIndex, const char *x);


/**
 * Sets the <i>in</i> parameter at index <code>parameterIndex</code> to the 
 * given int value. 
 * @param P A PreparedStatement object
 * @param parameterIndex The first parameter is 1, the second is 2,..
 * @param x The int value to set
 * @exception SQLException if a database access error occurs or if parameter 
 * index is out of range
 * @see SQLException.h
 */
void PreparedStatement_setInt(T P, int parameterIndex, int x);


/**
 * Sets the <i>in</i> parameter at index <code>parameterIndex</code> to the 
 * given long long value. 
 * @param P A PreparedStatement object
 * @param parameterIndex The first parameter is 1, the second is 2,..
 * @param x The long long value to set
 * @exception SQLException if a database access error occurs or if parameter 
 * index is out of range
 * @see SQLException.h
 */
void PreparedStatement_setLLong(T P, int parameterIndex, long long int x);


/**
 * Sets the <i>in</i> parameter at index <code>parameterIndex</code> to the 
 * given double value. 
 * @param P A PreparedStatement object
 * @param parameterIndex The first parameter is 1, the second is 2,..
 * @param x The double value to set
 * @exception SQLException if a database access error occurs or if parameter 
 * index is out of range
 * @see SQLException.h
 */
void PreparedStatement_setDouble(T P, int parameterIndex, double x);


/**
 * Sets the <i>in</i> parameter at index <code>parameterIndex</code> to the 
 * given blob value. 
 * @param P A PreparedStatement object
 * @param parameterIndex The first parameter is 1, the second is 2,..
 * @param x The blob value to set
 * @param size The number of bytes in the blob 
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
 * @return A ResultSet object that contains the data produced by the prepared
 * statement.
 * @exception SQLException if a database error occurs
 * @see ResultSet.h
 * @see SQLException.h
 */
ResultSet_T PreparedStatement_executeQuery(T P);


#undef T
#endif
