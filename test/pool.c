#include "Config.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>

#include "URL.h"
#include "Thread.h"
#include "Vector.h"
#include "ResultSet.h"
#include "PreparedStatement.h"
#include "Connection.h"
#include "ConnectionPool.h"
#include "AssertException.h"
#include "SQLException.h"


/**
 * libzdb connection pool unity tests. 
 */
#define BSIZE 2048

#define SCHEMA_MYSQL      "CREATE TABLE zild_t(id INTEGER AUTO_INCREMENT PRIMARY KEY, name VARCHAR(255), percent REAL, image BLOB);"
#define SCHEMA_POSTGRESQL "CREATE TABLE zild_t(id SERIAL PRIMARY KEY, name VARCHAR(255), percent REAL, image BYTEA);"
#define SCHEMA_SQLITE     "CREATE TABLE zild_t(id INTEGER PRIMARY KEY, name VARCHAR(255), percent REAL, image BLOB);"

const char *schema;

static void TabortHandler(const char *error) {
        fprintf(stdout, "Error: %s\n", error);
        exit(1);
}

void testPool(const char *testURL) {
        URL_T url;
        ConnectionPool_T pool;
        char *data[]= {"Fry", "Leela", "Bender", "Farnsworth",
                "Zoidberg", "Amy", "Hermes", "Nibbler", "Cubert",
                "Zapp", "Joey Mousepad", "ЯΣ༆", NULL}; 
        
        if (Str_startsWith(testURL,        "mysql")) {
                schema = SCHEMA_MYSQL;
        } else if (Str_startsWith(testURL, "postgresql")) {
                schema = SCHEMA_POSTGRESQL;
        } else if (Str_startsWith(testURL, "sqlite")) {
                schema = SCHEMA_SQLITE;
        } else {
                exit(1);
        }

        printf("=> Test1: create/destroy\n");
        {
                pool= ConnectionPool_new(URL_new(testURL));
                assert(pool);
                url= ConnectionPool_getURL(pool);
                ConnectionPool_free(&pool);
                assert(pool==NULL);
                URL_free(&url);
        }
        printf("=> Test1: OK\n\n");
        
        printf("=> Test2: NULL value\n");
        {
                url= URL_new(NULL);
                assert(! url);
                pool= ConnectionPool_new(url);
                assert(! pool);
        }
        printf("=> Test2: OK\n\n");
        
        printf("=> Test3: start/stop\n");
        {
                url= URL_new(testURL);
                pool= ConnectionPool_new(url);
                assert(pool);
                ConnectionPool_start(pool);
                ConnectionPool_stop(pool);
                ConnectionPool_free(&pool);
                assert(pool==NULL);
                URL_free(&url);
        }
        printf("=> Test3: OK\n\n");
        
        printf("=> Test4: Connection execute & transaction\n");
        {
                int i;
                Connection_T con;
                url= URL_new(testURL);
                pool= ConnectionPool_new(url);
                assert(pool);
                ConnectionPool_setAbortHandler(pool, TabortHandler);
                ConnectionPool_start(pool);
                con= ConnectionPool_getConnection(pool);
                assert(con);
                TRY Connection_execute(con, "drop table zild_t;"); ELSE END_TRY;
                Connection_execute(con, schema);
                Connection_beginTransaction(con);
                /* Insert values into database and assume that auto increment of id works */
                for (i= 0; data[i]; i++) 
                        Connection_execute(con, "insert into zild_t (name, percent) values('%s', %d.%d);", data[i], i+1, i);
                // Assert that the last insert statement added one row
                assert(Connection_rowsChanged(con) == 1);
                /* Assert that last row id works for MySQL and SQLite. PostgreSQL does not
                 support last row id directly. The way to do this in PostgreSQL is to use 
                 currval() or return the id on insert. Its just frakked up */
                if (IS(URL_getProtocol(url), "sqlite") || IS(URL_getProtocol(url), "mysql")) 
                        assert(Connection_lastRowId(con) == 12);
                Connection_commit(con);
                printf("\tResult: table zild_t successfully created\n");
                Connection_close(con);
        }
        printf("=> Test4: OK\n\n");     
        
        
        printf("=> Test5: Prepared Statement\n");
        {
                int i,j;
                char blob[65533];
                Connection_T con;
                PreparedStatement_T pre;
                char *data[]= {"Ceci n'est pas une pipe", "Mona Lisa", 
                        "Bryllup i Hardanger", "The Scream",
                        "Vampyre", "Balcony", "Cycle", "Day & Night", 
                        "Hand with Reflecting Sphere",
                        "Drawing Hands", "Ascending and Descending", 0}; 
                con= ConnectionPool_getConnection(pool);
                assert(con);
                pre= Connection_prepareStatement(con, "update zild_t set image=? where id=?;");
                assert(pre);
                for (i= 0; data[i]; i++) {
                        PreparedStatement_setBlob(pre, 1, data[i], strlen(data[i])+1);
                        PreparedStatement_setInt(pre, 2, i + 1);
                        PreparedStatement_execute(pre);
                }
                /* Add a database null value */
                PreparedStatement_setBlob(pre, 1, NULL, 0);
                PreparedStatement_setInt(pre, 2, 5);
                PreparedStatement_execute(pre);
                /* Add a large blob */
                for (j= 0; j<65532; j+=4)
                        snprintf(&blob[j], 5, "%s", "blob"); 
                /* Mark start and end */
                *blob='S'; blob[strlen(blob)-1]= 'E';
                PreparedStatement_setBlob(pre, 1, blob, strlen(blob)+1);
                PreparedStatement_setInt(pre, 2, i + 1);
                PreparedStatement_execute(pre);
                printf("\tResult: prepared statement successfully executed\n");
                Connection_close(con);
        }
        printf("=> Test5: OK\n\n");     
        
        
        printf("=> Test6: Result Sets\n");
        {
                int i;
                int imagesize= 0;
                Connection_T con;
                ResultSet_T rset;
                ResultSet_T names;
                PreparedStatement_T pre;
                con= ConnectionPool_getConnection(pool);
                assert(con);
                rset= Connection_executeQuery(con, "select id, name, percent, image from zild_t where id < %d order by id;", 100);
                assert(rset);
                printf("\tResult:\n");
                printf("\tNumber of columns in resultset: %d\n\t", ResultSet_getColumnCount(rset));
                assert(4==ResultSet_getColumnCount(rset));
                i= 1;
                printf("%-5s", ResultSet_getColumnName(rset, i++));
                printf("%-16s", ResultSet_getColumnName(rset, i++));
                printf("%-10s", ResultSet_getColumnName(rset, i++));
                printf("%-16s", ResultSet_getColumnName(rset, i++));
                printf("\n\t------------------------------------------------------\n");
                while (ResultSet_next(rset)) {
                        int id= ResultSet_getIntByName(rset, "id");
                        const char *name= ResultSet_getString(rset, 2);
                        double percent= ResultSet_getDoubleByName(rset, "percent");
                        const char *blob= (char*)ResultSet_getBlob(rset, 4, &imagesize);
                        printf("\t%-5d%-16s%-10.2f%-16.38s\n", id, name?name:"null", percent, blob&&imagesize?blob:"");
                }
                rset= Connection_executeQuery(con, "select image from zild_t where id=12;");
                assert(1==ResultSet_getColumnCount(rset));
                if (ResultSet_next(rset)) {
                        int n= 0;
                        long off= 0;
                        unsigned char buf[8193];
                        printf("\tResult: reading a large blob of size(%ld)\n", ResultSet_getColumnSize(rset, 1));
                        while ((n= ResultSet_readData(rset, 1, buf, 8192, off))>0) {
                                buf[n]= 0;
                                /* Uncomment for full blob */
                                //printf("%s", buf);
                                fprintf(stdout, "\t\tCHUNK SIZE=%d\n", n);
                                off+= n;
                        }
                }
                printf("\tResult: check max rows..");
                Connection_setMaxRows(con, 3);
                rset= Connection_executeQuery(con, "select id from zild_t;");
                assert(rset);
                i= 0;
                while (ResultSet_next(rset)) i++;
                assert((i)==3);
                printf("success\n");
                printf("\tResult: check prepared statement resultset..");
                Connection_setMaxRows(con, 0);
                pre= Connection_prepareStatement(con, "select name from zild_t where id=?");
                assert(pre);
                PreparedStatement_setInt(pre, 1, 2);
                names= PreparedStatement_executeQuery(pre);
                assert(names);
                assert(ResultSet_next(names));
                assert(Str_isEqual("Leela", ResultSet_getString(names, 1)));
                printf("success\n");
                printf("\tResult: check prepared statement re-execute..");
                PreparedStatement_setInt(pre, 1, 1);
                names= PreparedStatement_executeQuery(pre);
                assert(names);
                assert(ResultSet_next(names));
                assert(Str_isEqual("Fry", ResultSet_getString(names, 1)));
                printf("success\n");
                printf("\tResult: check prepared statement without in-params..");
                pre= Connection_prepareStatement(con, "select name from zild_t;");
                assert(pre);
                names= PreparedStatement_executeQuery(pre);
                assert(names);
                i= 0;
                while (ResultSet_next(names)) i++;
                assert(i==12);
                printf("success\n");
                /* Need to close and release statements before 
                   we can drop the table, sqlite need this */
                Connection_clear(con);
                Connection_execute(con, "drop table zild_t;");
                Connection_close(con);
                ConnectionPool_stop(pool);
                ConnectionPool_free(&pool);
                assert(pool==NULL);
                URL_free(&url);
        }
        printf("=> Test6: OK\n\n");     

        printf("=> Test7: reaper start/stop\n");
        {
                int i;
                Vector_T v= Vector_new(20);
                url= URL_new(testURL);
                pool= ConnectionPool_new(url);
                assert(pool);
                ConnectionPool_setInitialConnections(pool, 4);
                ConnectionPool_setMaxConnections(pool, 20);
                ConnectionPool_setConnectionTimeout(pool, 4);
                ConnectionPool_setReaper(pool, 4);
                ConnectionPool_setAbortHandler(pool, TabortHandler);
                ConnectionPool_start(pool);
                assert(4==ConnectionPool_size(pool));
                printf("Creating 20 Connections..");
                for (i= 0; i<20; i++)
                        Vector_push(v, ConnectionPool_getConnection(pool));
                assert(20==ConnectionPool_size(pool));
                assert(ConnectionPool_active(pool)==20);
                printf("success\n");
                printf("Closing Connections down to initial..");
                while (! Vector_isEmpty(v))
                        Connection_close(Vector_pop(v));
                assert(ConnectionPool_active(pool)==0);
                printf("success\n");
                printf("Please wait 10 sec for reaper to harvest closed connections..");
                fflush(stdout);
                sleep(10);
                assert(4==ConnectionPool_size(pool));
                printf("success\n");
                ConnectionPool_stop(pool);
                ConnectionPool_free(&pool);
                Vector_free(&v);
                assert(pool==NULL);
                URL_free(&url);
        }
        printf("=> Test7: OK\n\n");

        printf("=> Test8: Exceptions handling\n");
        {
                int i;
                Connection_T con;
                ResultSet_T result;
                url= URL_new(testURL);
                pool= ConnectionPool_new(url);
                assert(pool);
                ConnectionPool_setAbortHandler(pool, TabortHandler);
                ConnectionPool_start(pool);
                con= ConnectionPool_getConnection(pool);
                assert(con);
                /* 
                 * The following should work without throwing exceptions 
                 */
                TRY
                {
                        Connection_execute(con, schema);
                }
                ELSE
                {
                        printf("\tResult: Creating table zild_t failed -- %s\n", Connection_getLastError(con));
                        assert(false); // Should not fail
                }
                END_TRY;
                TRY
                {
                        Connection_beginTransaction(con);
                        for (i= 0; data[i]; i++) 
                                Connection_execute(con, "insert into zild_t (name, percent) values('%s', %d.%d);", data[i], i+1, i);
                        Connection_commit(con);
                        printf("\tResult: table zild_t successfully created\n");
                }
                ELSE
                {
                        printf("\tResult: Test failed -- %s\n", Exception_frame.message);
                        assert(false); // Should not fail
                }
                FINALLY
                {
                        Connection_close(con);
                }
                END_TRY;
                assert((con= ConnectionPool_getConnection(pool)));
                TRY
                {
                        int i, j;
                        const char *bg[]= {"Starbuck", "Sharon Valerii",
                                "Number Six", "Gaius Baltar", "William Adama",
                                "Lee \"Apollo\" Adama", "Laura Roslin", 0};
                        PreparedStatement_T p = Connection_prepareStatement
                        (con, "insert into zild_t (name) values(?);");
                        /* If we did not get a statement, an SQLException is thrown
                           and we will not get here. So we can safely use the 
                           statement now. Likewise, below, we do not have to 
                           check return values from the statement since any error
                           will throw an SQLException and transfer the control
                           to the exception handler
                        */
                        for (i= 0, j= 42; bg[i]; i++, j++) {
                                PreparedStatement_setString(p, 1, bg[i]);
                                PreparedStatement_execute(p);
                        }
                }
                CATCH(SQLException)
                {
                        printf("\tResult: prepare statement failed -- %s\n", Exception_frame.message);
                        assert(false);
                }
                END_TRY;
                TRY
                {
                        printf("\t\tBattlestar Galactica: \n");
                        result= Connection_executeQuery(con, "select name from zild_t where id < 20;");
                        while (ResultSet_next(result))
                                printf("\t\t%s\n", ResultSet_getString(result, 1));
                }
                CATCH(SQLException)
                {
                        printf("\tResult: resultset failed -- %s\n", Exception_frame.message);
                       assert(false);
                }
                FINALLY
                {
                        Connection_close(con);
                }
                END_TRY;
                assert((con= ConnectionPool_getConnection(pool)));
                /* 
                 * The following should fail and throw exceptions. The exception error 
                 * message can be obtained with Exception_frame.message, or from 
                 * Connection_getLastError(con). Exception_frame.message contains both
                 * SQL errors or api errors such as prepared statement parameter index
                 * out of range, while Connection_getLastError(con) only has SQL errors
                 */
                TRY
                {
                        Connection_execute(con, schema);
                        /* Creating the table again should fail and we 
                        should not come here */
                        assert(false);
                }
                CATCH(SQLException)
                {
                        printf("\tResult: Ok got SQLException -- %s\n", Exception_frame.message);
                }
                END_TRY;
                TRY
                {
                        printf("\tResult: query with errors.. ");
                        Connection_executeQuery(con, "blablabala;");
                        assert(false);
                }
                CATCH(SQLException)
                {
                        printf("ok got SQLException -- %s\n", Exception_frame.message);
                }
                END_TRY;
                TRY
                {
                        printf("\tResult: Column index out of range.. ");
                        result= Connection_executeQuery(con, "select id, name from zild_t;");
                        while (ResultSet_next(result)) {
                                int id= ResultSet_getInt(result, 1);  
                                const char *name= ResultSet_getString(result, 2);
                                /* So far so good, now, try access an invalid
                                   column, which should throw an SQLException */
                                int bogus= ResultSet_getInt(result, 3);
                                assert(false); // Should not come here
                                printf("%d, %s, %d", id, name, bogus);
                        }
                }
                CATCH(SQLException)
                {
                        printf("ok got SQLException -- %s\n", Exception_frame.message);
                }
                END_TRY;
                TRY
                {
                        PreparedStatement_T p;
                        p= Connection_prepareStatement(con, "update zild_t set name = ? where id = ?;");
                        printf("\tResult: Parameter index out of range.. ");
                        PreparedStatement_setInt(p, 3, 123);
                        assert(false);
                }
                CATCH(SQLException)
                {
                        printf("ok got SQLException -- %s\n", Exception_frame.message);
                }
                FINALLY
                {
                        Connection_close(con);
                }
                END_TRY;
                assert((con= ConnectionPool_getConnection(pool)));
                Connection_execute(con, "drop table zild_t;");
                Connection_close(con);
                ConnectionPool_stop(pool);
                ConnectionPool_free(&pool);
                assert(pool==NULL);
                URL_free(&url);
        }
        printf("=> Test8: OK\n\n");    
        
        printf("============> Connection Pool Tests: OK\n\n");
}

int main(void) {
        URL_T url;
        char buf[BSIZE];
        char *help= "Please enter a valid database connection URL and press ENTER\n"
                    "E.g. sqlite:///tmp/sqlite.db?synchronous=off&show_datatypes=off\n"
                    "E.g. mysql://localhost:3306/test?user=root&password=root\n"
                    "E.g. postgresql://localhost:5432/test?user=root&password=root\n"
                    "To exit, enter '.' on a single line\n\nConnection URL> ";
        ZBDEBUG= true;
        Exception_init();
        printf("============> Start Connection Pool Tests\n\n");
        printf("This test will create and drop a table called zild_t in the database\n");
	printf("%s", help);
	while (fgets(buf, BSIZE, stdin)) {
		if (*buf == '.')
                        break;
		if (*buf == '\r' || *buf == '\n' || *buf == 0) 
			goto next;
                url= URL_new(buf);
                if (url==NULL) {
                        printf("Please enter a valid database URL or stop by entering '.'\n");
                        goto next;
                }
                testPool(URL_toString(url));
                URL_free(&url);
                printf("%s", help);
                continue;
next:
                printf("Connection URL> ");
	}
	return 0;
}
