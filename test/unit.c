#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

#include "Config.h"
#include "URL.h"
#include "Vector.h"
#include "system/Time.h"
#include "StringBuffer.h"


/**
 * libzdb support classes unit tests. 
 */

static void vectorVisitor(const void *element, void *ap) {
        printf("%c", *(char *)element);
        (*((int*)ap))++;
}

int abortHandlerCalled = 0;
static void abortHandler(const char *error) {
        abortHandlerCalled = 1;
}


static void testStr() {
        printf("============> Start Str Tests\n\n");
        
        printf("=> Test1: copy\n");
        {
                char s3[STRLEN];
                printf("\tResult: %s\n", Str_copy(s3, "The abc house",7));
                assert(Str_isEqual(s3, "The abc"));
                printf("\tTesting for NULL argument\n");
                assert(! Str_copy(NULL, NULL, 7));
        }
        printf("=> Test1: OK\n\n");
        
        printf("=> Test2: dup\n");
        {
                char *s4 = Str_dup("abc123");
                printf("\tResult: %s\n", s4);
                assert(Str_isEqual(s4, "abc123"));
                printf("\tTesting for NULL argument\n");
                assert(! Str_dup(NULL));
                FREE(s4);
        }
        printf("=> Test2: OK\n\n");
        
        printf("=> Test3: ndup\n");
        {
                char *s5 = Str_ndup("abc123", 3);
                printf("\tResult: %s\n", s5);
                assert(Str_isEqual(s5, "abc"));
                printf("\tTesting for NULL argument\n");
                assert(! Str_ndup(NULL, 3));
                FREE(s5);
        }
        printf("=> Test3: OK\n\n");
        
        printf("=> Test4: Str_cat & Str_vcat\n");
        {
                char *s6;
                s6 = Str_cat("%s://%s%s?%s", "https", "foo.bar", 
                            "/uri", "abc=123");
                printf("\tResult: %s\n", s6);
                assert(Str_isEqual(s6, "https://foo.bar/uri?abc=123"));
                FREE(s6);
                printf("\tTesting for NULL arguments\n");
                s6 = Str_cat(NULL);
                assert(s6 == NULL);
                FREE(s6);
        }
        printf("=> Test4: OK\n\n");
        
        printf("=> Test5: startsWith\n");
        {
                char *a = "mysql://localhost:3306/zild?user=root&password=swordfish";
                printf("\tResult: starts with mysql - %s\n", 
                       Str_startsWith(a, "mysql")?"yes":"no");
                assert(Str_startsWith(a, "mysql"));
                assert(! Str_startsWith(a, "sqlite"));
                assert(Str_startsWith("sqlite", "sqlite"));
                printf("\tTesting for NULL and NUL argument\n");
                assert(! Str_startsWith(a, NULL));
                assert(! Str_startsWith(a, ""));
                assert(! Str_startsWith(NULL, "mysql"));
                assert(! Str_startsWith("", NULL));
                assert(! Str_startsWith(NULL, NULL));
                assert(Str_startsWith("", ""));
        }
        printf("=> Test5: OK\n\n");
        
        printf("=> Test6: parseInt, parseLong, parseLLong, parseFloat, parseDouble\n");
        {
                char i[STRLEN] = "   -2812 bla";
                char ll[STRLEN ] = "  2147483642 blabla";
                char d[STRLEN] = "  2.71828182845904523536028747135266249775724709369995 this is e";
                char de[STRLEN] = "9.461E^99 nanometers";
                char ie[STRLEN] = " 9999999999999999999999999999999999999";
                printf("\tResult:\n");
                printf("\tParsed int32 = %d\n", Str_parseInt(i));
                printf("\tParsed int64 = %lld\n", Str_parseLLong(ll));
                printf("\tParsed double = %.52f\n", Str_parseDouble(d));
                printf("\tParsed double exp = %.3e\n", Str_parseDouble(de));
                TRY
                {
                        printf("\tParse overflow int = %d\n", Str_parseInt(ie));
                        assert(false); //Should not come here
                }
                CATCH(SQLException)
                END_TRY;
                TRY
                {
                        printf("\tParse NaN = %d\n", Str_parseInt("blabla"));
                        assert(false); //Should not come here
                }
                CATCH(SQLException)
                END_TRY;
        }
        printf("=> Test6: OK\n\n");
        
        
        printf("============> Str Tests: OK\n\n");
}


static void testMem() {
        printf("============> Start Mem Tests\n\n");
        
        printf("=> Test1: alloc\n");
        {
                char *s7 = ALLOC(2048);
                assert(Str_isEqual(Str_copy(s7, "123456789", 2048), "123456789"));
                FREE(s7);
        }
        printf("=> Test1: OK\n\n");
        
        printf("=> Test2: calloc\n");
        {
                char *s8 = CALLOC(2, 1024);
                assert(s8[2047] == '\0');
                FREE(s8);
        }
        printf("=> Test2: OK\n\n");
        
        printf("=> Test3: resize\n");
        {
                char *s9 = ALLOC(4);
                Str_copy(s9, "abc", 3);
                assert(Str_isEqual(s9, "abc"));
                RESIZE(s9, 7);
                Str_copy(s9, "abc123", 6);
                assert(Str_isEqual(s9, "abc123"));
                FREE(s9);
        }
        printf("=> Test3: OK\n\n");
        
        printf("============> Mem Tests: OK\n\n");
}


static void testTime() {
        printf("============> Start Time Tests\n\n");
                
        printf("=> Test1: now\n");
        {
                printf("\tResult: %ld\n", Time_now());
        }
        printf("=> Test1: OK\n\n");

        printf("=> Test2: milli\n");
        {
                printf("\tResult: %lld\n", Time_milli());
        }
        printf("=> Test2: OK\n\n");
        
        printf("=> Test3: Time_toString\n");
        {
                char *t = Time_toString(1387010338, (char[20]){});
                assert(Str_isEqual(t, "2013-12-14 08:38:58"));
                printf("\tResult: %s\n", t);
                // Assert time is converted to UTC
                Time_toString(0, t);
                assert(Str_isEqual(t, "1970-01-01 00:00:00"));
                printf("\tResult: %s\n", t);
        }
        printf("=> Test3: OK\n\n");
        
        printf("=> Test4: Time_toDateTime\n");
        {
#if HAVE_STRUCT_TM_TM_GMTOFF
#define TM_GMTOFF tm_gmtoff
#else
#define TM_GMTOFF tm_wday
#endif
                struct tm t;
                // DateTime ISO-8601 format
                assert(Time_toDateTime("2013-12-14T09:38:08Z", &t));
                assert(t.tm_year == 2013);
                assert(t.tm_mon  == 11);
                assert(t.tm_mday == 14);
                assert(t.tm_hour == 9);
                assert(t.tm_min  == 38);
                assert(t.tm_sec  == 8);
                // Date
                assert(Time_toDateTime("2013-12-14", &t));
                assert(t.tm_year == 2013);
                assert(t.tm_mon  == 11);
                assert(t.tm_mday == 14);
                // Time
                assert(Time_toDateTime("09:38:08", &t));
                assert(t.tm_hour == 9);
                assert(t.tm_min  == 38);
                assert(t.tm_sec  == 8);
                // Compressed DateTime
                assert(Time_toDateTime(" 20131214093808", &t));
                assert(t.tm_year == 2013);
                assert(t.tm_mon  == 11);
                assert(t.tm_mday == 14);
                assert(t.tm_hour == 9);
                assert(t.tm_min  == 38);
                assert(t.tm_sec  == 8);
                // Compressed Date
                assert(Time_toDateTime(" 20131214 ", &t));
                assert(t.tm_year == 2013);
                assert(t.tm_mon  == 11);
                assert(t.tm_mday == 14);
                // Compressed Time
                assert(Time_toDateTime("093808", &t));
                assert(t.tm_hour == 9);
                assert(t.tm_min  == 38);
                assert(t.tm_sec  == 8);
                // Reverse DateTime
                assert(Time_toDateTime(" 09:38:08 2013-12-14", &t));
                assert(t.tm_year == 2013);
                assert(t.tm_mon  == 11);
                assert(t.tm_mday == 14);
                assert(t.tm_hour == 9);
                assert(t.tm_min  == 38);
                assert(t.tm_sec  == 8);
                // DateTime with timezone Zulu (UTC)
                assert(Time_toDateTime("The Battle of Stamford Bridge 1066-09-25 12:15:33+00:00", &t));
                assert(t.tm_year == 1066);
                assert(t.tm_mon  == 8);
                assert(t.tm_mday == 25);
                assert(t.tm_hour == 12);
                assert(t.tm_min  == 15);
                assert(t.tm_sec  == 33);
                assert(t.TM_GMTOFF == 0); // offset from UTC in seconds
                // Time with timezone
                assert(Time_toDateTime(" 09:38:08+01:45", &t));
                assert(t.tm_hour == 9);
                assert(t.tm_min  == 38);
                assert(t.tm_sec  == 8);
                assert(t.TM_GMTOFF == 6300);
                // Time with timezone PST compressed
                assert(Time_toDateTime("Pacific Time Zone 09:38:08 -0800 ", &t));
                assert(t.tm_hour == 9);
                assert(t.tm_min  == 38);
                assert(t.tm_sec  == 8);
                assert(t.TM_GMTOFF == -28800);
                // Date without time, tz should not be set
                assert(Time_toDateTime("2013-12-15-0800 ", &t));
                assert(t.TM_GMTOFF == 0);
                // Invalid date
                TRY {
                        Time_toDateTime("1901-123-45", &t);
                        printf("\t Test Failed\n");
                        exit(1);
                } CATCH (SQLException) {
                        // OK
                } ELSE {
                        printf("\t Test Failed with wrong exception\n");
                        exit(1);
                }
                END_TRY;
        }
        printf("=> Test4: OK\n\n");
        
        printf("=> Test5: Time_toTimestamp\n");
        {
                // Time, fraction of second is ignored. No timezone in string means UTC
                time_t t = Time_toTimestamp("2013-12-15 00:12:58.123456");
                assert(t == 1387066378);
                // TimeZone east
                t = Time_toTimestamp("Tokyo timezone: 2013-12-15 09:12:58+09:00");
                assert(t == 1387066378);
                // TimeZone west
                t = Time_toTimestamp("New York timezone: 2013-12-14 19:12:58-05:00");
                assert(t == 1387066378);
                // TimeZone east with hour and minute offset
                t = Time_toTimestamp("Nepal timezone: 2013-12-15 05:57:58+05:45");
                assert(t == 1387066378);
                // TimeZone Zulu
                t = Time_toTimestamp("Grenwich timezone: 2013-12-15 00:12:58Z");
                assert(t == 1387066378);
                // Compressed
                t = Time_toTimestamp("20131214191258-0500");
                assert(t == 1387066378);
                // Invalid timestamp string
                TRY {
                        Time_toTimestamp("1901-123-45 10:12:14");
                        // Should not come here
                        printf("\t Test Failed\n");
                        exit(1);
                } CATCH (SQLException) {
                        // OK
                } ELSE {
                        printf("\t Test Failed with wrong exception\n");
                        exit(1);
                }
                END_TRY;
        }
        printf("=> Test5: OK\n\n");
        
        printf("=> Test6: usleep\n");
        {
                Time_usleep(1);
        }
        printf("=> Test6: OK\n\n");
        
        printf("============> Time Tests: OK\n\n");
}


static void testSystem() {
        printf("============> Start System Tests\n\n");
        
        printf("=> Test1: debug\n");
        {
                ZBDEBUG = true;
                DEBUG("\tResult: %s\n", ABOUT);
        }
        printf("=> Test1: OK\n\n");
        
        printf("=> Test2: abort\n");
        {
                AbortHandler = abortHandler;
                ABORT("\tResult: %s\n", ABOUT);
                assert(abortHandlerCalled);
                AbortHandler = NULL; // Reset so exceptions are thrown in later tests
        }
        printf("=> Test2: OK\n\n");

        printf("=> Test3: getLastError & getError\n");
        {
                int fd = open("l_d$#askfjlsdkfjlskfjdlskfjdlskfjaldkjf", 0);
                assert(fd <= 0);
                assert(System_getLastError());
                assert(System_getError(errno));
        }
        printf("=> Test3: OK\n\n");
        
        printf("============> System Tests: OK\n\n");
}


static void testURL() {
        URL_T url;
        printf("============> Start URL Tests\n\n");
        
        printf("=> Test1: create/destroy\n");
        {
                url = URL_new("http://");
                assert(url);
                URL_free(&url);
        }
        printf("=> Test1: OK\n\n");
        
        printf("=> Test2: NULL value\n");
        {
                url = URL_new(NULL);
                assert(! url);
        }
        printf("=> Test2: OK\n\n");
        
        printf("=> Test3: empty string\n");
        {
                url = URL_new("");
                assert(! url);
        }
        printf("=> Test3: OK\n\n");
        
        printf("=> Test4: non-url string\n");
        {
                url = URL_new("quad est demonstrandum");
                assert(! url);
        }
        printf("=> Test4: OK\n\n");
        
        printf("=> Test5: Parse full url string\n");
        {
                // Wrap in cruft to demonstrate that only the URL is parsed
                url = URL_create("\t\n quad et <a href=http://hauk:admin@www.foo.bar%s",
                                ":8080/document/index.csp?query=string&name=libzdb#ref>ipsum demonstrandum</a>");
                assert(url);
                printf("\tResult:\n");
                printf("\tprotocol: %s\n", URL_getProtocol(url));
                assert(Str_isEqual("http", URL_getProtocol(url)));
                printf("\tuser: %s\n", URL_getUser(url));
                assert(Str_isEqual("hauk", URL_getUser(url)));
                printf("\tpassword: %s\n", URL_getPassword(url));
                assert(Str_isEqual("admin", URL_getPassword(url)));
                printf("\thost: %s\n", URL_getHost(url));
                assert(Str_isEqual("www.foo.bar", URL_getHost(url)));
                printf("\tport: %d\n", URL_getPort(url));
                assert(8080 == URL_getPort(url));
                printf("\tpath: %s\n", URL_getPath(url));
                assert(Str_isEqual("/document/index.csp", URL_getPath(url)));
                printf("\tquery: %s\n", URL_getQueryString(url));
                assert(Str_isEqual("query=string&name=libzdb", URL_getQueryString(url)));
                printf("\tparameter name=%s\n", URL_getParameter(url, "name"));
                assert(Str_isEqual("libzdb", URL_getParameter(url, "name")));
                printf("\tparameter query=%s\n", URL_getParameter(url, "query"));
                assert(Str_isEqual("string", URL_getParameter(url, "query")));
                assert(Str_isEqual("http://hauk:admin@www.foo.bar:8080/document/index.csp?query=string&name=libzdb", URL_toString(url)));
                URL_free(&url);
                // Test URL_toString without explicit port
                URL_T u = URL_new("http://www.tildeslash.com/");
                assert(Str_isEqual("http://www.tildeslash.com/", URL_toString(u)));
                URL_free(&u);
        }
        printf("=> Test5: OK\n\n");
        
        printf("=> Test6: only http:// protocol string\n");
        {
                url = URL_new("http://");
                assert(url);
                printf("\tResult: %s\n", URL_toString(url));
                assert(Str_isEqual(URL_toString(url), "http://"));
                URL_free(&url);
        }
        printf("=> Test6: OK\n\n");
        
        printf("=> Test7: file:///etc/passwd\n");
        {
                url = URL_new("file:///etc/passwd");
                assert(url);
                printf("\tResult: %s\n", URL_toString(url));
                assert(Str_isEqual("file:///etc/passwd", URL_toString(url)));
                printf("\tPath: %s\n", URL_getPath(url));
                assert(Str_isEqual("/etc/passwd", URL_getPath(url)));
                URL_free(&url);
        }
        printf("=> Test7: OK\n\n");
        
        printf("=> Test8: unescape\n");
        {
                char s9a[] = "";
		char s9x[] = "http://www.tildeslash.com/%";
		char s9y[] = "http://www.tildeslash.com/%0";
                char s9[] = "http://www.tildeslash.com/zild/%20zild%5B%5D.doc";
                printf("\tResult: %s\n", URL_unescape(s9));
                assert(Str_isEqual(s9, "http://www.tildeslash.com/zild/ zild[].doc"));
                assert(Str_isEqual(URL_unescape(s9a), ""));
                printf("\tTesting for NULL argument\n");
                assert(! URL_unescape(NULL));
		// Test guard against invalid url encoding
                assert(Str_isEqual(URL_unescape(s9x), "http://www.tildeslash.com/"));
                assert(Str_isEqual(URL_unescape(s9y), "http://www.tildeslash.com/"));
        }
        printf("=> Test8: OK\n\n");
        
        printf("=> Test9: escape\n");
        {
                char s10[] = "http://www.tildeslash.com/<>#%{}|\\^~[] `";
                char *rs10 = URL_escape(s10);
                printf("\tResult: %s -> \n\t%s\n", s10, rs10);
                assert(Str_isEqual(rs10, "http://www.tildeslash.com/%3C%3E%23%25%7B%7D%7C%5C%5E~%5B%5D%20%60"));
                printf("\tTesting for NULL argument\n");
                assert(! URL_escape(NULL));
                FREE(rs10);
        }
        printf("=> Test9: OK\n\n");
        
        printf("=> Test10: URL parameters,\n\tmysql://localhost:3306/db?"
               "user=root&password=swordfish&charset=utf8\n");
        {
                int i = 0;
                const char **params;
                url = URL_create("mysql://localhost:3306/db?"
                                "user=root&password=swordfish&charset=utf8");
                assert(url);
                printf("\tParameter result:\n");
                params = URL_getParameterNames(url);
                if (params) for (i = 0; params[i]; i++)
                        printf("\t\t%s:%s\n", 
                               params[i], URL_getParameter(url, params[i]));
                assert(i == 3);
                URL_free(&url);
        }
        printf("=> Test10: OK\n\n");
           
        printf("=> Test11: auto unescape of credentials, path and param values\n");
        {
                url = URL_new("mysql://r%40ot:p%40ssword@localhost/test%20dir?user=r%26ot&password=pass%3Dword");
                assert(IS(URL_getUser(url), "r@ot"));
                assert(IS(URL_getPassword(url), "p@ssword"));
                assert(IS(URL_getPath(url), "/test dir"));
                assert(IS(URL_getParameter(url, "user"), "r&ot"));
                assert(IS(URL_getParameter(url, "password"), "pass=word"));
                URL_free(&url);
        }
        printf("=> Test11: OK\n\n");

        printf("=> Test12: IPv4 and IPv6 hosts\n");
        {
                // IPv4
                url = URL_new("mysql://r%40ot:p%40ssword@192.168.2.42:3306/test?user=r%26ot&password=pass%3Dword");
                assert(IS(URL_getUser(url), "r@ot"));
                assert(IS(URL_getPassword(url), "p@ssword"));
                assert(IS(URL_getHost(url), "192.168.2.42"));
                assert(URL_getPort(url) == 3306);
                assert(IS(URL_getPath(url), "/test"));
                assert(IS(URL_getParameter(url, "user"), "r&ot"));
                assert(IS(URL_getParameter(url, "password"), "pass=word"));
                URL_free(&url);
                // IPv6
                url = URL_new("mysql://r%40ot:p%40ssword@[2001:db8:85a3::8a2e:370:7334]:3306/test?user=r%26ot&password=pass%3Dword");
                assert(IS(URL_getUser(url), "r@ot"));
                assert(IS(URL_getPassword(url), "p@ssword"));
                // Assert that IP6 brackets are removed
                assert(IS(URL_getHost(url), "2001:db8:85a3::8a2e:370:7334"));
                assert(URL_getPort(url) == 3306);
                assert(IS(URL_getPath(url), "/test"));
                assert(IS(URL_getParameter(url, "user"), "r&ot"));
                assert(IS(URL_getParameter(url, "password"), "pass=word"));
                // Assert that IP6 brackets are put back in toString
                assert(IS(URL_toString(url), "mysql://r@ot:p@ssword@[2001:db8:85a3::8a2e:370:7334]:3306/test?user=r%26ot&password=pass%3Dword"));
                URL_free(&url);
                // IPv6 localhost
                url = URL_new("mysql://[fe80::1%lo0]/");
                // Assert that IP6 brackets are removed
                assert(IS(URL_getHost(url), "fe80::1%lo0"));
                assert(URL_getPort(url) == 3306);
                // Assert that IP6 brackets are put back in toString
                assert(IS(URL_toString(url), "mysql://[fe80::1%lo0]/"));
                URL_free(&url);
        }
        printf("=> Test12: OK\n\n");

        printf("============> URL Tests: OK\n\n");
}


static void testVector() {
        Vector_T vector;
        printf("============> Start Vector Tests\n\n");
        
        printf("=> Test1: create/destroy\n");
        {
                vector = Vector_new(0);
                assert(vector);
                assert(Vector_size(vector) == 0);
                Vector_free(&vector);
                assert(vector == NULL);
        }
        printf("=> Test1: OK\n\n");
        
        printf("=> Test1: push & get\n");
        {
                int i;
                char b[] = "abcdefghijklmnopqrstuvwxyz";
                vector = Vector_new(1);
                assert(vector);
                for (i = 0; i < 10; i++)
                        Vector_push(vector, &b[i]);
                assert(Vector_size(vector) == 10);
                assert(*(char*)Vector_get(vector, 7) =='h');
                Vector_free(&vector);
                assert(vector == NULL);
        }
        printf("=> Test1: OK\n\n");
        
        printf("=> Test2: insert & get\n");
        {
                int i;
                char b[] = "abcdefghijklmnopqrstuvwxyz";
                vector = Vector_new(1);
                assert(vector);
                for (i = 0; i < 10; i++)
                        Vector_insert(vector, i, &b[i]);
                assert(Vector_size(vector) == 10);
                assert(*(char*)Vector_get(vector, 7) =='h');
                Vector_insert(vector, 5, &b[21]);
                assert(*(char*)Vector_get(vector, 8) =='h');
                assert(*(char*)Vector_get(vector, 5) =='v');
                assert(*(char*)Vector_get(vector, 4) =='e');
                printf("\tResult: ");
                for (i = 0; i<Vector_size(vector); i++)
                        printf("%c", *(char*)Vector_get(vector, i));
                printf("\n");
                assert(Vector_size(vector) == 11);
                Vector_free(&vector);
                assert(vector == NULL);
        }
        printf("=> Test2: OK\n\n");
        
        printf("=> Test3: push & remove\n");
        {
                int i;
                char b[] = "abcdefghijklmnopqrstuvwxyz";
                vector = Vector_new(1);
                assert(vector);
                for (i = 0; i < 11; i++)
                        Vector_push(vector, &b[i]);
                assert(Vector_size(vector) == 11);
                for (i = 5; i>=0; i--)
                        assert(b[i] == *(char*)Vector_remove(vector, i));
                assert(Vector_size(vector) == 5);
                printf("\tResult: ");
                for (i = 0; i<Vector_size(vector); i++)
                        printf("%c", *(char*)Vector_get(vector, i));
                printf("\n");
                assert('g'==*(char*)Vector_get(vector, 0));
                for (i = Vector_size(vector)-1; i>=0; i--)
                        Vector_remove(vector, i);
                assert(Vector_size(vector) == 0);
                Vector_free(&vector);
                assert(vector == NULL);
        }
        printf("=> Test3: OK\n\n");
        
        printf("=> Test4: push & set\n");
        {
                int i,j;
                char b[] = "abcde";
                vector = Vector_new(1);
                assert(vector);
                for (i = 0; i < 5; i++)
                        Vector_push(vector, &b[i]);
                assert(Vector_size(vector) == 5);
                assert('b'==*(char*)Vector_get(vector, 1));
                for (j =0,i =4; i>=0; i--,j++)
                        Vector_set(vector, j, &b[i]);
                assert(Vector_size(vector) == 5);
                assert('e'==*(char*)Vector_get(vector, 0) && 
                       'a'==*(char*)Vector_get(vector, Vector_size(vector)-1));
                printf("\tResult: ");
                for (i = 0; i<Vector_size(vector); i++)
                        printf("%c", *(char*)Vector_get(vector, i));
                printf("\n");
                Vector_free(&vector);
                assert(vector == NULL);
        }
        printf("=> Test4: OK\n\n");        
   
        printf("=> Test5: map\n");
        {
                int i,j;
                char b[] = "abcdefghijklmnopqrstuvwxyz";
                vector = Vector_new(1);
                assert(vector);
                for (i = 0; i < 10; i++)
                        Vector_push(vector, &b[i]);
                assert(Vector_size(vector) == 10);
                j = 10;
                printf("\tResult: ");
                Vector_map(vector, vectorVisitor, &j);
                printf("\n");
                assert(j == 20);
                Vector_free(&vector);
                assert(vector == NULL);
        }
        printf("=> Test5: OK\n\n");        
        
        printf("=> Test6: toArray\n");
        {
                int i;
                void **array;
                char b[] = "abcdefghijklmnopqrstuvwxyz";
                vector = Vector_new(9);
                assert(vector);
                for (i = 0; i < 26; i++)
                        Vector_push(vector, &b[i]);
                assert(Vector_size(vector) == 26);
                array = Vector_toArray(vector);
                printf("\tResult: ");
                for (i = 0; array[i]; i++)
                        printf("%c", *(char*)array[i]);
                printf("\n");
                FREE(array);
                Vector_free(&vector);
                assert(vector == NULL);
        }
        printf("=> Test6: OK\n\n");        
        
        printf("=> Test7: pop & isEmpty\n");
        {
                int i;
                char b[] = "abcdefghijklmnopqrstuvwxyz";
                vector = Vector_new(100);
                assert(vector);
                for (i = 0; i<26; i++)
                        Vector_push(vector, &b[i]);
                assert(Vector_size(vector) == 26);
                printf("\tResult: ");
                while (! Vector_isEmpty(vector))
                        printf("%c", *(char*)Vector_pop(vector));
                printf("\n");
                assert(Vector_isEmpty(vector));
                Vector_free(&vector);
                assert(vector == NULL);
        }
        printf("=> Test7: OK\n\n");        

        printf("============> Vector Tests: OK\n\n");
}


static void testStringBuffer() {
        StringBuffer_T sb;
        printf("============> Start StringBuffer Tests\n\n");
        
        printf("=> Test1: create/destroy\n");
        {
                sb = StringBuffer_new("");
                assert(sb);
                assert(StringBuffer_length(sb) == 0);
                StringBuffer_free(&sb);
                assert(sb == NULL);
                sb = StringBuffer_create(4);
                assert(sb);
                assert(StringBuffer_length(sb) == 0);
                StringBuffer_free(&sb);
                assert(sb == NULL);
        }
        printf("=> Test1: OK\n\n");
        
        printf("=> Test2: append NULL value\n");
        {
                sb = StringBuffer_new("");
                assert(sb);
                StringBuffer_append(sb, NULL);
                assert(StringBuffer_length(sb) == 0);
                StringBuffer_free(&sb);
                assert(sb == NULL);
        }
        printf("=> Test2: OK\n\n");
        
        printf("=> Test3: create with string\n");
        {
                sb = StringBuffer_new("abc");
                assert(sb);
                assert(StringBuffer_length(sb) == 3);
                StringBuffer_free(&sb);
                assert(sb == NULL);
        }
        printf("=> Test3: OK\n\n");
        
        printf("=> Test4: append string value\n");
        {
                sb = StringBuffer_new("abc");
                assert(sb);
                StringBuffer_append(sb, "def");
                assert(StringBuffer_length(sb) == 6);
                StringBuffer_free(&sb);
                assert(sb == NULL);
                // Test with create
                sb = StringBuffer_create(4);
                assert(sb);
                StringBuffer_append(sb, "abc");
                assert(StringBuffer_length(sb) == 3);
                StringBuffer_append(sb, "def");
                assert(StringBuffer_length(sb) == 6);
                StringBuffer_free(&sb);
                assert(sb == NULL);
        }
        printf("=> Test4: OK\n\n");

        printf("=> Test4: toString value\n");
        {
                sb = StringBuffer_new("abc");
                assert(sb);
                StringBuffer_append(sb, "def");
                assert(Str_isEqual(StringBuffer_toString(sb), "abcdef"));
                StringBuffer_free(&sb);
                assert(sb == NULL);
        }
        printf("=> Test4: OK\n\n");

        printf("=> Test5: internal resize\n");
        {
                int i;
                sb = StringBuffer_new("");
                assert(sb);
                for (i = 0; i<1024; i++)
                        StringBuffer_append(sb, "a");
                assert(StringBuffer_length(sb) == 1024);
                assert(StringBuffer_toString(sb)[1023] == 'a');
                assert(StringBuffer_toString(sb)[1024] == 0);
                StringBuffer_free(&sb);
                assert(sb == NULL);
        }
        printf("=> Test5: OK\n\n");
        
        printf("=> Test6: prepare4postgres and prepare4oracle\n");
        {
                // Nothing to replace
                sb = StringBuffer_new("select * from host;");
                assert(StringBuffer_prepare4postgres(sb) == 0);
                assert(Str_isEqual(StringBuffer_toString(sb), "select * from host;"));
                StringBuffer_free(&sb);
                assert(sb == NULL);
                // Replace n < 10 using $
                sb = StringBuffer_new("insert into host values(?, ?, ?);");
                assert(StringBuffer_prepare4postgres(sb) == 3);
                assert(Str_isEqual(StringBuffer_toString(sb), "insert into host values($1, $2, $3);"));
                StringBuffer_free(&sb);
                assert(sb == NULL);
                // Replace n < 10 using :
                sb = StringBuffer_new("insert into host values(?, ?, ?);");
                assert(StringBuffer_prepare4oracle(sb) == 3);
                assert(Str_isEqual(StringBuffer_toString(sb), "insert into host values(:1, :2, :3);"));
                StringBuffer_free(&sb);
                assert(sb == NULL);
                // Replace n > 10
                sb = StringBuffer_new("insert into host values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
                assert(StringBuffer_prepare4postgres(sb) == 12);
                assert(Str_isEqual(StringBuffer_toString(sb), "insert into host values($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12);"));
                StringBuffer_free(&sb);
                assert(sb == NULL);
                // Replace n > 99, should throw exception
                sb = StringBuffer_new("insert into host values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
                assert(sb);
                TRY
                {
                        StringBuffer_prepare4postgres(sb);
                        assert(!"Should not come here");
                }
                CATCH(SQLException)
                {
                        StringBuffer_free(&sb);
                        assert(sb == NULL);
                }
                END_TRY;
                // Just 99 ?'s
                sb = StringBuffer_new("???????????????????????????????????????????????????????????????????????????????????????????????????");
                assert(StringBuffer_prepare4postgres(sb) == 99);
                assert(Str_isEqual(StringBuffer_toString(sb), "$1$2$3$4$5$6$7$8$9$10$11$12$13$14$15$16$17$18$19$20$21$22$23$24$25$26$27$28$29$30$31$32$33$34$35$36$37$38$39$40$41$42$43$44$45$46$47$48$49$50$51$52$53$54$55$56$57$58$59$60$61$62$63$64$65$66$67$68$69$70$71$72$73$74$75$76$77$78$79$80$81$82$83$84$85$86$87$88$89$90$91$92$93$94$95$96$97$98$99"));
                StringBuffer_free(&sb);
                assert(sb == NULL);
        }
        printf("=> Test6: OK\n\n");
        
        printf("=> Test7: trim\n");
        {
                // Empty buffer
                sb = StringBuffer_create(256);
                StringBuffer_trim(sb);
                assert(Str_isEqual(StringBuffer_toString(sb), ""));
                StringBuffer_free(&sb);
                assert(sb == NULL);
                // White space filled buffer
                sb = StringBuffer_new("     ");
                StringBuffer_trim(sb);
                assert(Str_isEqual(StringBuffer_toString(sb), ""));
                StringBuffer_free(&sb);
                assert(sb == NULL);
                // Nothing to remove
                sb = StringBuffer_new("select a from b");
                StringBuffer_trim(sb);
                assert(Str_isEqual(StringBuffer_toString(sb), "select a from b"));
                StringBuffer_free(&sb);
                assert(sb == NULL);
                // Remove white space
                sb = StringBuffer_new("\t select a from b; \r\n");
                StringBuffer_trim(sb);
                assert(Str_isEqual(StringBuffer_toString(sb), "select a from b;"));
                StringBuffer_free(&sb);
                assert(sb == NULL);
        }
        printf("=> Test7: OK\n\n");
        
        printf("=> Test8: set value\n");
        {
                sb= StringBuffer_new("abc");
                assert(sb);
                StringBuffer_set(sb, "def");
                assert(IS(StringBuffer_toString(sb), "def"));
                StringBuffer_free(&sb);
                assert(sb==NULL);
                printf("\tTesting set and internal resize:...");
                sb= StringBuffer_create(4);
                assert(sb);
                StringBuffer_set(sb, "abc");
                StringBuffer_set(sb, "abcdef");
                assert(IS(StringBuffer_toString(sb), "abcdef"));
                StringBuffer_set(sb, " ");
                assert(IS(StringBuffer_toString(sb), " "));
                printf("ok\n");
                StringBuffer_free(&sb);
                assert(sb==NULL);
        }
        printf("=> Test8: OK\n\n");

        printf("============> StringBuffer Tests: OK\n\n");
}


int main(void) {
        Exception_init();
	testStr();
	testMem();
	testTime();
	testSystem();
	testURL();
        testVector();
        testStringBuffer();
	return 0;
}
