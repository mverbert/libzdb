#include <stdio.h>

#include <assert.h>
#include <zdb.h>

/*
 This example demonstrate most of the functionality of libzdb and can be compiled with a C, OBJ-C(++) or a C++ compiler.
 Compile: [gcc -std=c99|g++|clang|clang++] -o select select.c -L/<libzdb>/lib -lzdb -I/<libzdb>/include/zdb
 */

int main(void) {
        URL_T url = URL_new("sqlite:///tmp/test.db");
        ConnectionPool_T pool = ConnectionPool_new(url);
        ConnectionPool_start(pool);
        Connection_T con = ConnectionPool_getConnection(pool);
        TRY
        {
                Connection_execute(con, "create table bleach(name varchar(255))");
                PreparedStatement_T p = Connection_prepareStatement(con, "insert into bleach values (?)"); 
                const char *bleach[] = {
                        "Ichigo Kurosaki", "Rukia Kuchiki", "Orihime Inoue",  "Yasutora \"Chad\" Sado", 
                        "Kisuke Urahara", "Uryū Ishida", "Renji Abarai", 0
                };
                for (int i = 0; bleach[i]; i++) {
                        PreparedStatement_setString(p, 1, bleach[i]);
                        PreparedStatement_execute(p);
                }
                ResultSet_T r = Connection_executeQuery(con, "select name from bleach");
                while (ResultSet_next(r))
                        printf("%s\n", ResultSet_getString(r, 1));
                Connection_execute(con, "drop table bleach;");
        }
        CATCH(SQLException)
        {
                printf("SQLException -- %s\n", Exception_frame.message);
        }
        FINALLY
        {
                Connection_close(con);
        }
        END_TRY;
        ConnectionPool_free(&pool);
        URL_free(&url);
        return 0;
}
