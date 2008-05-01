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

const char *blob1 = "From nobody@pacific.net.sg Tue Dec 04 19:52:17 2007\n"
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

const char *blob2 = "(\"Tue, 06 Aug 2002 19:54:41 +0200\" \"[dovecot] mbox support\" ((\"Marcus Rueckert\" NIL \"rueckert\" \"informatik.uni-rostock.de\")) ((NIL NIL \"dovecot-bounce\" \"procontrol.fi\")) ((\"Marcus Rueckert\" NIL \"rueckert\" \"informatik.uni-rostock.de\")) ((\"dovecot mailing list\" NIL \"dovecot\" \"procontrol.fi\")) NIL NIL NIL \"<0000420020806175441.GA7148@linux.taugt.net>\")";


//#define DMTEST
#define BUFSIZE 8192
int main(void)
{
	const char *in = blob1;
	int i = 0;
	ResultSet_T res;
	PreparedStatement_T s;
        ZBDEBUG=1;
	URL_T url = URL_new("mysql://root:root@localhost:3306/test");
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
                for (i=0; i<20; i++) {
                        PreparedStatement_setString(s,1,in);
                        PreparedStatement_execute(s);
                }
                
                res = Connection_executeQuery(con, "SELECT id,data FROM test LIMIT 10");
                while(ResultSet_next(res)) {
                        const char *out = ResultSet_getString(res,2);
                        if (strcmp(in, out) != 0) {
                                printf("Error mismatch\n[%s]\n[%s]\n", in, out);
                        } else {
                                printf("Row matches\n");
                        }
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
