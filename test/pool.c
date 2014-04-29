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
#define SCHEMA_ORACLE     "CREATE TABLE zild_t(id NUMBER , name VARCHAR(255), percent REAL, image CLOB);"

#if HAVE_STRUCT_TM_TM_GMTOFF
#define TM_GMTOFF tm_gmtoff
#else
#define TM_GMTOFF tm_wday
#endif


static void TabortHandler(const char *error) {
        fprintf(stdout, "Error: %s\n", error);
        exit(1);
}

static void testPool(const char *testURL) {
        URL_T url;
        char *schema;
        ConnectionPool_T pool;
        char *data[]= {"Fry", "Leela", "Bender", "Farnsworth",
                "Zoidberg", "Amy", "Hermes", "Nibbler", "Cubert",
                "Zapp", "Joey Mousepad", "ЯΣ༆", 0}; 
        
        if (Str_startsWith(testURL, "mysql")) {
                schema = SCHEMA_MYSQL;
        } else if (Str_startsWith(testURL, "postgresql")) {
                schema = SCHEMA_POSTGRESQL;
        } else if (Str_startsWith(testURL, "sqlite")) {
                schema = SCHEMA_SQLITE;
        } else if (Str_startsWith(testURL, "oracle")) {
                schema = SCHEMA_ORACLE;
        }
        else {
                printf("Unsupported database protocol\n");
                exit(1);
        }

        
        printf("=> Test1: create/destroy\n");
        {
                pool = ConnectionPool_new(URL_new(testURL));
                assert(pool);
                url = ConnectionPool_getURL(pool);
                ConnectionPool_free(&pool);
                assert(! pool);
                URL_free(&url);
        }
        printf("=> Test1: OK\n\n");
        
        printf("=> Test2: NULL value\n");
        {
                url = URL_new(NULL);
                assert(! url);
                TRY
                {
                        pool = ConnectionPool_new(url);
                        printf("\tResult: Test failed -- exception not thrown\n");
                        exit(1);
                }
                CATCH(AssertException)
                {
                        // OK
                }
                END_TRY;
        }
        printf("=> Test2: OK\n\n");
        
        printf("=> Test3: start/stop\n");
        {
                url = URL_new(testURL);
                pool = ConnectionPool_new(url);
                assert(pool);
                ConnectionPool_start(pool);
                ConnectionPool_stop(pool);
                ConnectionPool_free(&pool);
                assert(pool==NULL);
                URL_free(&url);
                // Test that exception is thrown on start error
                TRY
                {
                        url = URL_new("not://a/database");
                        pool = ConnectionPool_new(url);
                        assert(pool);
                        ConnectionPool_start(pool);
                        printf("\tResult: Test failed -- exception not thrown\n");
                        exit(1);
                }
                CATCH(SQLException) {
                        // OK
                }
                FINALLY {
                        ConnectionPool_free(&pool);
                        assert(pool==NULL);
                        URL_free(&url);
                }
                END_TRY;
        }
        printf("=> Test3: OK\n\n");
        
        printf("=> Test4: Connection execute & transaction\n");
        {
                int i;
                Connection_T con;
                url = URL_new(testURL);
                pool = ConnectionPool_new(url);
                assert(pool);
                ConnectionPool_setAbortHandler(pool, TabortHandler);
                ConnectionPool_start(pool);
                con = ConnectionPool_getConnection(pool);
                assert(con);
                TRY Connection_execute(con, "drop table zild_t;"); ELSE END_TRY;
                Connection_execute(con, "%s", schema);
                Connection_beginTransaction(con);
                /* Insert values into database and assume that auto increment of id works */
                for (i = 0; data[i]; i++) 
                        Connection_execute(con, "insert into zild_t (name, percent) values('%s', %d.%d);", data[i], i+1, i);
                // Assert that the last insert statement added one row
                assert(Connection_rowsChanged(con) == 1);
                /* Assert that last row id works for MySQL and SQLite. Neither Oracle nor PostgreSQL
                 support last row id directly. The way to do this in PostgreSQL is to use 
                 currval() or return the id on insert. */
                if (IS(URL_getProtocol(url), "sqlite") || IS(URL_getProtocol(url), "mysql")) 
                        assert(Connection_lastRowId(con) == 12);
                Connection_commit(con);
                printf("\tResult: table zild_t successfully created\n");
                Connection_close(con);
        }
        printf("=> Test4: OK\n\n");     
        
        
        printf("=> Test5: Prepared Statement\n");
        {
                int i;
                char blob[8192];
                char *images[]= {"Ceci n'est pas une pipe", "Mona Lisa",
                        "Bryllup i Hardanger", "The Scream",
                        "Vampyre", "Balcony", "Cycle", "Day & Night", 
                        "Hand with Reflecting Sphere",
                        "Drawing Hands", "Ascending and Descending", 0}; 
                Connection_T con = ConnectionPool_getConnection(pool);
                assert(con);
                // 1. Prepared statement, perform a nonsense update to test rowsChanged
                PreparedStatement_T p1 = Connection_prepareStatement(con, "update zild_t set image=?;");
                PreparedStatement_setString(p1, 1, "");
                PreparedStatement_execute(p1);
                printf("\tRows changed: %lld\n", PreparedStatement_rowsChanged(p1));
                // Assert that all 12 rows in the data set was changed
                assert(PreparedStatement_rowsChanged(p1) == 12);
                // 2. Prepared statement, update the table proper with "images". 
                PreparedStatement_T pre = Connection_prepareStatement(con, "update zild_t set image=? where id=?;");
                assert(pre);
                assert(PreparedStatement_getParameterCount(pre) == 2);
                for (i = 0; images[i]; i++) {
                        PreparedStatement_setBlob(pre, 1, images[i], (int)strlen(images[i])+1);
                        PreparedStatement_setInt(pre, 2, i + 1);
                        PreparedStatement_execute(pre);
                }
                // The last execute changed one row only
                assert(PreparedStatement_rowsChanged(pre) == 1);
                /* Add a database null blob value for id = 5 */
                PreparedStatement_setBlob(pre, 1, NULL, 0);
                PreparedStatement_setInt(pre, 2, 5);
                PreparedStatement_execute(pre);
                /* Add a database null string value for id = 1 */
                PreparedStatement_setString(pre, 1, NULL);
                PreparedStatement_setInt(pre, 2, 1);
                PreparedStatement_execute(pre);
                /* Add a large blob */
                memset(blob, 'x', 8192);
                blob[8191] = 0;
                /* Mark start and end */
                *blob='S'; blob[8190] = 'E';
                PreparedStatement_setBlob(pre, 1, blob, 8192);
                PreparedStatement_setInt(pre, 2, i + 1);
                PreparedStatement_execute(pre);
                printf("\tResult: prepared statement successfully executed\n");
                Connection_close(con);
        }
        printf("=> Test5: OK\n\n");     
        
        
        printf("=> Test6: Result Sets\n");
        {
                int i;
                int imagesize = 0;
                Connection_T con;
                ResultSet_T rset;
                ResultSet_T names;
                PreparedStatement_T pre;
                con = ConnectionPool_getConnection(pool);
                assert(con);
                rset = Connection_executeQuery(con, "select id, name, percent, image from zild_t where id < %d order by id;", 100);
                assert(rset);
                printf("\tResult:\n");
                printf("\tNumber of columns in resultset: %d\n\t", ResultSet_getColumnCount(rset));
                assert(4==ResultSet_getColumnCount(rset));
                i = 1;
                printf("%-5s", ResultSet_getColumnName(rset, i++));
                printf("%-16s", ResultSet_getColumnName(rset, i++));
                printf("%-10s", ResultSet_getColumnName(rset, i++));
                printf("%-16s", ResultSet_getColumnName(rset, i++));
                printf("\n\t------------------------------------------------------\n");
                while (ResultSet_next(rset)) {
                        int id = ResultSet_getIntByName(rset, "id");
                        const char *name = ResultSet_getString(rset, 2);
                        double percent = ResultSet_getDoubleByName(rset, "percent");
                        const char *blob = (char*)ResultSet_getBlob(rset, 4, &imagesize);
                        printf("\t%-5d%-16s%-10.2f%-16.38s\n", id, name ? name : "null", percent, imagesize ? blob : "");
                }
                // Column count
                rset = Connection_executeQuery(con, "select image from zild_t where id=12;");
                assert(1 == ResultSet_getColumnCount(rset));
                
                // Assert that types are interchangeable and that all data is returned
                while (ResultSet_next(rset)) {
                        const char *image = ResultSet_getStringByName(rset, "image");
                        const void *blob = ResultSet_getBlobByName(rset, "image", &imagesize);
                        assert(image && blob);
                        assert(strlen(image) + 1 == 8192);
                        assert(imagesize == 8192);
                }
                
                printf("\tResult: check isnull..");
                rset = Connection_executeQuery(con, "select id, image from zild_t where id in(1,5,2);");
                while (ResultSet_next(rset)) {
                        int id = ResultSet_getIntByName(rset, "id");
                        if (id == 1 || id == 5)
                                assert(ResultSet_isnull(rset, 2) == true);
                        else
                                assert(ResultSet_isnull(rset, 2) == false);
                }
                printf("success\n");

                printf("\tResult: check max rows..");
                Connection_setMaxRows(con, 3);
                rset = Connection_executeQuery(con, "select id from zild_t;");
                assert(rset);
                i = 0;
                while (ResultSet_next(rset)) i++;
                assert((i)==3);
                printf("success\n");
                
                printf("\tResult: check prepared statement resultset..");
                Connection_setMaxRows(con, 0);
                pre = Connection_prepareStatement(con, "select name from zild_t where id=?");
                assert(pre);
                PreparedStatement_setInt(pre, 1, 2);
                names = PreparedStatement_executeQuery(pre);
                assert(names);
                assert(ResultSet_next(names));
                assert(Str_isEqual("Leela", ResultSet_getString(names, 1)));
                printf("success\n");
                
                printf("\tResult: check prepared statement re-execute..");
                PreparedStatement_setInt(pre, 1, 1);
                names = PreparedStatement_executeQuery(pre);
                assert(names);
                assert(ResultSet_next(names));
                assert(Str_isEqual("Fry", ResultSet_getString(names, 1)));
                printf("success\n");
                
                printf("\tResult: check prepared statement without in-params..");
                pre = Connection_prepareStatement(con, "select name from zild_t;");
                assert(pre);
                names = PreparedStatement_executeQuery(pre);
                assert(names);
                for (i = 0; ResultSet_next(names); i++);
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
                Vector_T v = Vector_new(20);
                url = URL_new(testURL);
                pool = ConnectionPool_new(url);
                assert(pool);
                ConnectionPool_setInitialConnections(pool, 4);
                ConnectionPool_setMaxConnections(pool, 20);
                ConnectionPool_setConnectionTimeout(pool, 4);
                ConnectionPool_setReaper(pool, 4);
                ConnectionPool_setAbortHandler(pool, TabortHandler);
                ConnectionPool_start(pool);
                assert(4==ConnectionPool_size(pool));
                printf("Creating 20 Connections..");
                for (i = 0; i<20; i++)
                        Vector_push(v, ConnectionPool_getConnection(pool));
                assert(ConnectionPool_size(pool) == 20);
                assert(ConnectionPool_active(pool) == 20);
                printf("success\n");
                printf("Closing Connections down to initial..");
                while (! Vector_isEmpty(v))
                        Connection_close(Vector_pop(v));
                assert(ConnectionPool_active(pool) == 0);
                assert(ConnectionPool_size(pool) == 20);
                printf("success\n");
                printf("Please wait 10 sec for reaper to harvest closed connections..");
                Connection_T con = ConnectionPool_getConnection(pool); // Activate one connection to verify the reaper does not close any active
                fflush(stdout);
                sleep(10);
                assert(5 == ConnectionPool_size(pool)); // 4 initial connections + the one active we got above
                assert(1 == ConnectionPool_active(pool));
                printf("success\n");
                Connection_close(con);
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
                url = URL_new(testURL);
                pool = ConnectionPool_new(url);
                assert(pool);
                ConnectionPool_setAbortHandler(pool, TabortHandler);
                ConnectionPool_start(pool);
                con = ConnectionPool_getConnection(pool);
                assert(con);
                /* 
                 * The following should work without throwing exceptions 
                 */
                TRY
                {
                        Connection_execute(con, "%s", schema);
                }
                ELSE
                {
                        printf("\tResult: Creating table zild_t failed -- %s\n", Exception_frame.message);
                        assert(false); // Should not fail
                }
                END_TRY;
                TRY
                {
                        Connection_beginTransaction(con);
                        for (i = 0; data[i]; i++) 
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
                assert((con = ConnectionPool_getConnection(pool)));
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
                        for (i = 0, j = 42; bg[i]; i++, j++) {
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
                        result = Connection_executeQuery(con, "select name from zild_t where id > 12;");
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
                /* 
                 * The following should fail and throw exceptions. The exception error 
                 * message can be obtained with Exception_frame.message, or from 
                 * Connection_getLastError(con). Exception_frame.message contains both
                 * SQL errors or api errors such as prepared statement parameter index
                 * out of range, while Connection_getLastError(con) only has SQL errors
                 */
                TRY
                {
                        assert((con = ConnectionPool_getConnection(pool)));
                        Connection_execute(con, "%s", schema);
                        /* Creating the table again should fail and we 
                        should not come here */
                        printf("\tResult: Test failed -- exception not thrown\n");
                        exit(1);
                }
                CATCH(SQLException)
                {
                        Connection_close(con);
                }
                END_TRY;
                TRY
                {
                        assert((con = ConnectionPool_getConnection(pool)));
                        printf("\tTesting: Query with errors.. ");
                        Connection_executeQuery(con, "blablabala;");
                        printf("\tResult: Test failed -- exception not thrown\n");
                        exit(1);
                }
                CATCH(SQLException)
                {
                        printf("ok\n");
                        Connection_close(con);
                }
                END_TRY;
                TRY
                {
                        printf("\tTesting: Prepared statement query with errors.. ");
                        assert((con = ConnectionPool_getConnection(pool)));
                        PreparedStatement_T p = Connection_prepareStatement(con, "blablabala;");
                        ResultSet_T r = PreparedStatement_executeQuery(p);
                        while(ResultSet_next(r));
                        printf("\tResult: Test failed -- exception not thrown\n");
                        exit(1);
                }
                CATCH(SQLException)
                {
                        printf("ok\n");
                        Connection_close(con);
                }
                END_TRY;
                TRY
                {
                        assert((con = ConnectionPool_getConnection(pool)));
                        printf("\tTesting: Column index out of range.. ");
                        result = Connection_executeQuery(con, "select id, name from zild_t;");
                        while (ResultSet_next(result)) {
                                int id = ResultSet_getInt(result, 1);  
                                const char *name = ResultSet_getString(result, 2);
                                /* So far so good, now, try access an invalid
                                   column, which should throw an SQLException */
                                int bogus = ResultSet_getInt(result, 3);
                                printf("\tResult: Test failed -- exception not thrown\n");
                                printf("%d, %s, %d", id, name, bogus);
                                exit(1);
                        }
                }
                CATCH(SQLException)
                {
                        printf("ok\n");
                        Connection_close(con);
                }
                END_TRY;
                TRY
                {
                        assert((con = ConnectionPool_getConnection(pool)));
                        printf("\tTesting: Invalid column name.. ");
                        result = Connection_executeQuery(con, "select name from zild_t;");
                        while (ResultSet_next(result)) {
                                const char *name = ResultSet_getStringByName(result, "nonexistingcolumnname");
                                printf("%s", name);
                                printf("\tResult: Test failed -- exception not thrown\n");
                                exit(1);
                        }
                }
                CATCH(SQLException)
                {
                        printf("ok\n");
                        Connection_close(con);
                }
                END_TRY;
                TRY
                {
                        assert((con = ConnectionPool_getConnection(pool)));
                        PreparedStatement_T p = Connection_prepareStatement(con, "update zild_t set name = ? where id = ?;");
                        printf("\tTesting: Parameter index out of range.. ");
                        PreparedStatement_setInt(p, 3, 123);
                        printf("\tResult: Test failed -- exception not thrown\n");
                        exit(1);
                }
                CATCH(SQLException)
                {
                        printf("ok\n");
                }
                FINALLY
                {
                        Connection_close(con);
                }
                END_TRY;
                assert((con = ConnectionPool_getConnection(pool)));
                Connection_execute(con, "drop table zild_t;");
                Connection_close(con);
                ConnectionPool_stop(pool);
                ConnectionPool_free(&pool);
                assert(pool==NULL);
                URL_free(&url);
        }
        printf("=> Test8: OK\n\n");
        
        printf("=> Test9: Ensure Capacity\n");
        {
                /* Check that MySQL ensureCapacity works for columns that exceed the preallocated buffer and that no truncation is done */
                if ( Str_startsWith(testURL, "mysql")) {
                        int myimagesize;
                        url = URL_new(testURL);
                        pool = ConnectionPool_new(url);
                        assert(pool);
                        ConnectionPool_start(pool);
                        Connection_T con = ConnectionPool_getConnection(pool);
                        assert(con);
                        Connection_execute(con, "CREATE TABLE zild_t(id INTEGER AUTO_INCREMENT PRIMARY KEY, image BLOB, string TEXT);");
                        PreparedStatement_T p = Connection_prepareStatement(con, "insert into zild_t (image, string) values(?, ?);");
                        char t[4096];
                        memset(t, 'x', 4096);
                        t[4095] = 0;
                        for (int i = 0; i < 4; i++) {
                                PreparedStatement_setBlob(p, 1, t, (i+1)*512); // store successive larger string-blobs to trigger realloc on ResultSet_getBlobByName
                                PreparedStatement_setString(p, 2, t);
                                PreparedStatement_execute(p);
                        }
                        ResultSet_T r = Connection_executeQuery(con, "select image, string from zild_t;");
                        for (int i = 0; ResultSet_next(r); i++) {
                                ResultSet_getBlobByName(r, "image", &myimagesize);
                                const char *image = ResultSet_getStringByName(r, "image"); // Blob as image should be terminated
                                const char *string = ResultSet_getStringByName(r, "string");
                                assert(myimagesize == (i+1)*512);
                                assert(strlen(image) == ((i+1)*512));
                                assert(strlen(string) == 4095);
                        }
                        p = Connection_prepareStatement(con, "select image, string from zild_t;");
                        r = PreparedStatement_executeQuery(p);
                        for (int i = 0; ResultSet_next(r); i++) {
                                ResultSet_getBlobByName(r, "image", &myimagesize);
                                const char *image = ResultSet_getStringByName(r, "image");
                                const char *string = (char*)ResultSet_getStringByName(r, "string");
                                assert(myimagesize == (i+1)*512);
                                assert(strlen(image) == ((i+1)*512));
                                assert(strlen(string) == 4095);
                        }
                        Connection_execute(con, "drop table zild_t;");
                        Connection_close(con);
                        ConnectionPool_stop(pool);
                        ConnectionPool_free(&pool);
                        URL_free(&url);
                }
        }
        printf("=> Test9: OK\n\n");
        
        printf("=> Test10: Date, Time, DateTime and Timestamp\n");
        {
                url = URL_new(testURL);
                pool = ConnectionPool_new(url);
                assert(pool);
                ConnectionPool_start(pool);
                Connection_T con = ConnectionPool_getConnection(pool);
                if (Str_startsWith(testURL, "postgres"))
                        Connection_execute(con, "create table zild_t(d date, t time, dt timestamp, ts timestamp)");
                else if (Str_startsWith(testURL, "oracle"))
                        Connection_execute(con, "create table zild_t(d date, t time, dt date, ts timestamp)");
                else
                        Connection_execute(con, "create table zild_t(d date, t time, dt datetime, ts timestamp)");
                PreparedStatement_T p = Connection_prepareStatement(con, "insert into zild_t values (?, ?, ?, ?)");
                PreparedStatement_setString(p, 1, "2013-12-28");
                PreparedStatement_setString(p, 2, "10:12:42");
                PreparedStatement_setString(p, 3, "2013-12-28 10:12:42");
                PreparedStatement_setTimestamp(p, 4, 1387066378);
                PreparedStatement_execute(p);
                ResultSet_T r = Connection_executeQuery(con, "select * from zild_t");
                if (ResultSet_next(r)) {
                        struct tm date = ResultSet_getDateTime(r, 1);
                        struct tm time = ResultSet_getDateTime(r, 2);
                        struct tm datetime = ResultSet_getDateTime(r, 3);
                        time_t timestamp = ResultSet_getTimestamp(r, 4);
                        struct tm timestampAsTm = ResultSet_getDateTime(r, 4);
                        // Check Date
                        assert(date.tm_hour == 0);
                        assert(date.tm_year == 2013);
                        assert(date.tm_mon == 11); // Remember month - 1
                        assert(date.tm_mday == 28);
                        assert(date.TM_GMTOFF == 0);
                        // Check Time
                        assert(time.tm_year == 0);
                        assert(time.tm_hour == 10);
                        assert(time.tm_min == 12);
                        assert(time.tm_sec == 42);
                        assert(time.TM_GMTOFF == 0);
                        // Check datetime
                        assert(datetime.tm_year == 2013);
                        assert(datetime.tm_mon == 11); // Remember month - 1
                        assert(datetime.tm_mday == 28);
                        assert(datetime.tm_hour == 10);
                        assert(datetime.tm_min == 12);
                        assert(datetime.tm_sec == 42);
                        assert(datetime.TM_GMTOFF == 0);
                        // Check timestamp
                        assert(timestamp == 1387066378);
                        // Check timestamp as datetime
                        assert(timestampAsTm.tm_year == 2013);
                        assert(timestampAsTm.tm_mon == 11); // Remember month - 1
                        assert(timestampAsTm.tm_mday == 15);
                        assert(timestampAsTm.tm_hour == 0);
                        assert(timestampAsTm.tm_min == 12);
                        assert(timestampAsTm.tm_sec == 58);
                        assert(timestampAsTm.TM_GMTOFF == 0);
                        // Result
                        printf("\tDate: %s, Time: %s, DateTime: %s\n\tTimestamp as numeric: %ld, Timestamp as string: %s\n",
                               ResultSet_getString(r, 1),
                               ResultSet_getString(r, 2),
                               ResultSet_getString(r, 3),
                               ResultSet_getTimestamp(r, 4),
                               ResultSet_getString(r, 4)); // SQLite will show both as numeric
                }
                Connection_execute(con, "drop table zild_t;");
                Connection_close(con);
                ConnectionPool_stop(pool);
                ConnectionPool_free(&pool);
                assert(pool==NULL);
                URL_free(&url);
        }
        printf("=> Test10: OK\n\n");


        printf("============> Connection Pool Tests: OK\n\n");
}

int main(void) {
        URL_T url;
        char buf[BSIZE];
        char *help = "Please enter a valid database connection URL and press ENTER\n"
                    "E.g. sqlite:///tmp/sqlite.db?synchronous=off&heap_limit=2000\n"
                    "E.g. mysql://localhost:3306/test?user=root&password=root\n"
                    "E.g. postgresql://localhost:5432/test?user=root&password=root\n"
                    "E.g. oracle://localhost:1526/test?user=scott&password=tiger\n"
                    "To exit, enter '.' on a single line\n\nConnection URL> ";
        ZBDEBUG = true;
        Exception_init();
        printf("============> Start Connection Pool Tests\n\n");
        printf("This test will create and drop a table called zild_t in the database\n");
	printf("%s", help);
	while (fgets(buf, BSIZE, stdin)) {
		if (*buf == '.')
                        break;
		if (*buf == '\r' || *buf == '\n' || *buf == 0) 
			goto next;
                url = URL_new(buf);
                if (! url) {
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
