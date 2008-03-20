#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <URL.h>
#include <ResultSet.h>
#include <PreparedStatement.h>
#include <Connection.h>
#include <ConnectionPool.h>
#include <SQLException.h>


/*
 
 CREATE TABLE `test` (
 `id` int(11) NOT NULL auto_increment,
 `data` longblob NOT NULL,
 PRIMARY KEY  (`id`)
 ) ENGINE=InnoDB AUTO_INCREMENT=9 DEFAULT CHARSET=latin1
 
 */

const char *blob = "From nobody@pacific.net.sg Tue Dec 04 19:52:17 2007\n"
"X-Envelope-From: <nobody@pacific.net.sg>\n"
"Received: from [127.0.0.1] (port=49353 helo=test11)\n"
"        by centos.nowhere.com with smtp (Exim 4.63)\n"
"        (envelope-from <nobody@pacific.net.sg>)\n"
"        id 1IzWJv-0000Ep-5f\n"
"        for wallace@nowhere.com; Tue, 04 Dec 2007 19:52:17 +0800\n"
"From: \"Wallace\" <nobody@pacific.net.sg>\n"
"To: wallace <wallace@nowhere.com>\n"
"Subject: Test 11\n"
"Message-Id: <E1IzWJv-0000Ep-5f@centos.nowhere.com>\n"
"Date: Tue, 04 Dec 2007 19:52:16 +0800\n"
"\n"
"\n"
"This line works, however,\n"
"From what I know, this line gets truncated\n"
"This line gets truncated\n"
"This other line get truncated too\n";


//#define DMTEST
int main(void)
{
	int ZBDEBUG=1, i = 0;
	ResultSet_T res;
	PreparedStatement_T s;
	URL_T url = URL_new("mysql://root:root@localhost/test");
	assert(url);
	ConnectionPool_T pool = ConnectionPool_new(url);
	assert(pool);
	ConnectionPool_start(pool);
	Connection_T con = ConnectionPool_getConnection(pool);
	assert(con);
        
	TRY
        {
                Connection_execute(con, "DELETE FROM test");
                
                s = Connection_prepareStatement(con, "INSERT INTO test (data) values ( ? )");
                PreparedStatement_setBlob(s,1,blob,strlen(blob));
                for (i=0; i<20; i++)
                        PreparedStatement_execute(s);
                
                res = Connection_executeQuery(con, "SELECT id,data FROM test LIMIT 10");
                while(ResultSet_next(res)) {
                        printf("[%s]\n", ResultSet_getString(res,2));
                }
        }
	CATCH(SQLException)
        {
                printf("SQLException: %s\n", Connection_getLastError(con));
        }
	FINALLY
        {
                Connection_close(con);
        }
	END_TRY;
        
	return 0;
}
