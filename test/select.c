#include <stdio.h>
#include <assert.h>

#include <URL.h>
#include <ResultSet.h>
#include <PreparedStatement.h>
#include <Connection.h>
#include <ConnectionPool.h>
#include <SQLException.h>

/*
 gcc -o select select.c -L/<libzdb>/lib -lzdb -lpthread -I/<libzdb>/include/zdb
 */

int main(void) {
        Connection_T con;
        URL_T url = URL_new("sqlite://");
        ConnectionPool_T pool = ConnectionPool_new(url);
        ConnectionPool_start(pool);
        con = ConnectionPool_getConnection(pool);
        TRY
        {
                int i;
                char *bleach[] = {
                        "Ichigo Kurosaki", "Rukia Kuchiki", "Orihime Inoue",  "Yasutora \"Chad\" Sado", 
                        "Kisuke Urahara", "Uryū Ishida", "Renji Abarai", 0
                };
                TRY Connection_execute(con, "drop table ztest;"); ELSE END_TRY;
                Connection_execute(con, "create table ztest(name varchar(255));");
                PreparedStatement_T p = Connection_prepareStatement(con, "insert into ztest values (?);"); 
                for (i = 0; bleach[i]; i++) {
                        PreparedStatement_setString(p, 1, bleach[i]);
                        PreparedStatement_execute(p);
                }
                p = Connection_prepareStatement(con, "select name from ztest where name like ?;"); 
                PreparedStatement_setString(p, 1, "%Inoue%");
                ResultSet_T result = PreparedStatement_executeQuery(p);
                while (ResultSet_next(result))
                        printf("%s\n", ResultSet_getString(result, 1));
                Connection_execute(con, "drop table ztest;");
        }
        CATCH(SQLException)
        {
                printf("SQLException -- %s\n", Exception_frame.message);
        }
        FINALLY
        {
                Connection_close(con);
                ConnectionPool_free(&pool);
                URL_free(&url);
        }
        END_TRY;
        return 0;
}
