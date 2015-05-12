# Copyright (C) Tildeslash Ltd. All rights reserved.

AC_PREREQ([2.53])

AC_INIT([libzdb], [3.1], [bugs-libzdb@tildeslash.com])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/Config.h])
AC_CONFIG_COMMANDS([libtool_patch],[test `uname` = "OpenBSD" && perl -p -i -e "s/deplibs_check_method=.*/deplibs_check_method=pass_all/g" libtool])


# ---------------------------------------------------------------------------
# Programs 
# ---------------------------------------------------------------------------

AC_PROG_CC
AC_PROG_CXX
AC_PROG_GCC_TRADITIONAL
AC_PATH_PROG([RE2C], [re2c], [no], [$PATH:/usr/local/bin:/usr/bin])
if test "x$RE2C" = "xno"; then
        # Require re2c unless URL.c and Time.c already are built
        if test ! -f src/net/URL.c -a ! -f src/system/Time.c; then
                AC_MSG_ERROR([Libzdb require re2c. Download re2c from http://re2c.org/ or use your package manager])
        fi
fi
AM_PROG_LEX
if test -z "${LEXLIB+set}"; then
        # Require flex unless lex.yy.c already is built
        if test ! -f ./tools/filterh/lex.yy.c; then
                AC_MSG_ERROR([flex is required. Download from http://www.gnu.org/software/flex/ or use your package manager])
        fi
fi

# ---------------------------------------------------------------------------
# Tools
# ---------------------------------------------------------------------------

if test ! -d ./tools/bin/
then
        mkdir ./tools/bin || AC_MSG_ERROR([Failed to create ./tools/bin])
fi
# Build tools in-place in a subshell
if test ! -f ./tools/filterh/lex.yy.c; then
        (cd ./tools/filterh && $LEX filterh.l && $CC lex.yy.c -o ../bin/filterh  && rm -f lex.yy.o) || AC_MSG_ERROR([Failed to build tools])
else
        (cd ./tools/filterh && $CC lex.yy.c -o ../bin/filterh && rm -f lex.yy.o) || AC_MSG_ERROR([Failed to build tools])
fi
# Assert that we succeded building filterh
test -f ./tools/bin/filterh || AC_MSG_ERROR([Failed to build tools])

# ---------------------------------------------------------------------------
# Libtool 
# ---------------------------------------------------------------------------

AC_PROG_LIBTOOL

# ---------------------------------------------------------------------------
# Build options
# ---------------------------------------------------------------------------

AC_ARG_ENABLE(profiling,
        AS_HELP_STRING([--enable-profiling],
                [Build with debug and profiling options]),
    [
        if test "x$enableval" = "xyes" ; then
                CFLAGS=`echo $CFLAGS|sed 's/-O.//'`
                CFLAGS="$CFLAGS -g -pg"
                profile="true"
        else
                profile="false"
        fi
    ],
    [
        profile="false"
    ]
)

AC_ARG_ENABLE(optimized,
        AS_HELP_STRING([--enable-optimized], 
                [Build software optimized]),
    [
        if test "x$enableval" = "xyes" ; then
		CFLAGS=`echo $CFLAGS|sed 's/-O.//'`
                CFLAGS=`echo $CFLAGS|sed 's/-g//'`
                CFLAGS="$CFLAGS -O3"
                OPTIMIZED=1
        else
                OPTIMIZED=0
        fi
    ],
    [
        OPTIMIZED=0
    ]
)

AC_ARG_ENABLE([zild],
        AS_HELP_STRING([--enable-zild], 
                [Reduce visibility of objects for linking with the zild app server. 
                Require gcc version >= 4.x or clang. This is an internal Tildeslash
                build option and should NOT be used by a third-party]),
     [
        if test "x$enableval" = "xyes" ; then
                zild_protect="true"
                AC_DEFINE([PACKAGE_PROTECTED], 1, [Define to 1 to package protect (hide) non-api objects])
                AC_DEFINE([ZILD_PACKAGE_PROTECTED], 1, [Define to 1 to hide objects for linking with zild])
        else
                zild_protect="false"
        fi
     ],[zild_protect="false"]
)
AM_CONDITIONAL([WITH_ZILD], test "xtrue" = "x$zild_protect")

AC_ARG_ENABLE([protected],
        AS_HELP_STRING([--enable-protected], 
        [Package protect non-API objects. Require gcc version >= 4.x or clang.
         This option hide objects in the DSO which are not part of the API and
         not to be exported. The rationale is to optimize the ABI and protect
         non-public methods of the DSO and thereby reduce the potential for 
         namespace conflicts for clients linking with the library. Recommend]),
     [
        if test "x$enableval" = "xyes" ; then
                protect="true"
                AC_DEFINE([PACKAGE_PROTECTED], 1, [Define to 1 to package protect (hide) non-api objects])
        else
             protect="false"
        fi
     ],[protect="false"]
)

AC_ARG_ENABLE([openssl],
        AS_HELP_STRING([--enable-openssl(=<path>)], 
                [Link libzdb with openssl. If database libraries were linked static,
                libzdb may have to link with openssl to support crypto and ssl 
                functionality in SQL client libraries. An optional path argument may
                be given to specify the top-level directory to search for openssl to
                link with]),
     [
        if test "x$enableval" = "xno" ; then
                openssl="false"
        else
                openssl="true"
                if test "x$enableval" = "xyes"; then
                        AC_CHECK_LIB([ssl], [SSL_library_init], [], [AC_MSG_ERROR([libssl not found])])
                        AC_CHECK_LIB([crypto], [SHA1_Init], [], [AC_MSG_ERROR([libcrypto not found])])
                else
                        AC_MSG_CHECKING([for openssl in $enableval])
                        LDFLAGS="-L$enableval/lib -lssl -lcrypto $LDFLAGS "
                        CPPFLAGS="-I$enableval/include $CPPFLAGS"
                        if test -r "$enableval/lib/libssl.a" -a -r "$enableval/lib/libcrypto.a"; then
                                AC_MSG_RESULT([ok])
                        else
                                AC_MSG_ERROR([openssl not found in $enableval])
                        fi
                fi
        fi
     ],[openssl="false"]
)

AC_ARG_ENABLE(sqliteunlock,
        AS_HELP_STRING([--enable-sqliteunlock], 
                [Enable the SQLite unlock notification API (requires SQLite >= 3.6.12 
                compiled with the SQLITE_ENABLE_UNLOCK_NOTIFY C-preprocessor symbol).
                Using this option will greatly improve SQLite concurrency when libzdb
                and SQLite are used from a multi-threaded program.]),
    [
        if test "x$enableval" = "xyes" ; then
                SQLITEUNLOCK=1
                CFLAGS="$CFLAGS -DSQLITEUNLOCK"
        else
                SQLITEUNLOCK=0
        fi
    ],
    [
        SQLITEUNLOCK=0
    ]
)

if test "xfalse" = "x$protect" -a "xfalse" = "x$zild_protect"; then
        zild_build="false"
        test_build="true"
        UNIT_TEST="test"
else
        zild_build="true"
        test_build="false"
        UNIT_TEST=""
fi
AC_SUBST(UNIT_TEST)

# ---------------------------------------------------------------------------
# Libraries 
# ---------------------------------------------------------------------------

AC_SEARCH_LIBS([pthread_create], [pthread], [], [AC_MSG_ERROR([POSIX thread library is required])])

# Database Libraries 

mysql="yes"
check_mysql_config() 
{
        AC_PATH_PROG([MYSQLCONFIG], [mysql_config], [no], [$PATH:/usr/local/bin:/usr/local/mysql/bin])
        if test "x$MYSQLCONFIG" = "xno" 
        then
                AC_MSG_WARN([mysql_config is required to build libzdb with mysql])
                mysql="no"
        fi
}
AC_MSG_CHECKING(for mysql)
AC_ARG_WITH([mysql], 
        AS_HELP_STRING([--with-mysql(=<path>)], 
                [Path is optional and if given should specify the full path to the MySQL
                 configure script, mysql_config. E.g. --with-mysql=/<path>/mysql_config]), 
        [
                if test "xno" = "x$with_mysql"; then
                        AC_MSG_RESULT([no])
                        mysql="no"
                else
                        AC_MSG_RESULT([yes])
                        AC_CHECK_FILE([$with_mysql], [MYSQLCONFIG=$with_mysql], [check_mysql_config])        
                fi
        ],
        [
                AC_MSG_RESULT([yes])
                check_mysql_config
        ])
if test "xyes" = "x$mysql"; then
        svd_CPPFLAGS=$CPPFLAGS
        svd_LDFLAGS=$LDFLAGS
        CPPFLAGS="`$MYSQLCONFIG --include` $CPPFLAGS"
        LDFLAGS="`$MYSQLCONFIG --libs` $LDFLAGS"
        AC_CHECK_HEADERS([mysql.h], [], [mysql="no"])
        if test "xyes" = "x$mysql"; then
                DBCPPFLAGS="$DBCPPFLAGS `$MYSQLCONFIG --include`"
                DBLDFLAGS="$DBLDFLAGS `$MYSQLCONFIG --libs`"
                AC_DEFINE([HAVE_LIBMYSQLCLIENT], 1, [Define to 1 to enable mysql])
        else
                CPPFLAGS=$svd_CPPFLAGS
                LDFLAGS=$svd_LDFLAGS
        fi
fi
AM_CONDITIONAL([WITH_MYSQL], test "xyes" = "x$mysql")

postgresql="yes"
check_postgres_config() 
{
        AC_PATH_PROG([PGCONFIG], [pg_config], [no], [$PATH:/usr/local/bin:/usr/local/pgsql/bin])
        if test "x$PGCONFIG" = "xno"
        then
                AC_MSG_WARN([pg_config is required to build libzdb with postgresql])
                postgresql="no"
        fi
}
AC_MSG_CHECKING(for postgresql)
AC_ARG_WITH([postgresql], 
        AS_HELP_STRING([--with-postgresql(=<path>)], 
                [Path is optional and if given should specify the full path to the PostgreSQL
                 configure script, pg_config. E.g. --with-postgresql=/<path>/pg_config]), 
        [
                if test "xno" = "x$with_postgresql"; then
                        AC_MSG_RESULT([no])
                        postgresql="no"
                else
                        AC_MSG_RESULT([yes])
                        AC_CHECK_FILE([$with_postgresql], [PGCONFIG=$with_postgresql],[check_postgres_config])
                fi
        ],
        [
                AC_MSG_RESULT([yes])
                check_postgres_config
        ])
if test "xyes" = "x$postgresql"; then
        svd_CPPFLAGS=$CPPFLAGS
        svd_LDFLAGS=$LDFLAGS
        CPPFLAGS="-I`$PGCONFIG --includedir` $CPPFLAGS"
        LDFLAGS="-L`$PGCONFIG --libdir` $LDFLAGS"
        AC_CHECK_HEADERS([libpq-fe.h], [], [postgresql="no"])
        if test "xyes" = "x$postgresql"; then
                DBCPPFLAGS="$DBCPPFLAGS -I`$PGCONFIG --includedir`"
                DBLDFLAGS="$DBLDFLAGS -L`$PGCONFIG --libdir` -lpq"
                AC_DEFINE([HAVE_LIBPQ], 1, [Define to 1 to enable postgresql])
        else
                CPPFLAGS=$svd_CPPFLAGS
                LDFLAGS=$svd_LDFLAGS
        fi
fi
AM_CONDITIONAL([WITH_POSTGRESQL], test "xyes" = "x$postgresql")


sqlite="yes"
AC_MSG_CHECKING(for SQLite3)
AC_ARG_WITH([sqlite], 
        AS_HELP_STRING([--with-sqlite=<path>], 
                [Path is optional and if given should specify the full path to the SQLite installation. 
                 E.g. /usr/local/sqlite3]), 
        [
                if test "xno" = "x$with_sqlite"; then
                        AC_MSG_RESULT([no])
                        sqlite="no"
                else
                        AC_MSG_RESULT([yes])
                        AC_CHECK_FILE([$with_sqlite],
                        [
                                svd_LDFLAGS=$LDFLAGS
                                svd_CPPFLAGS=$CPPFLAGS
                                LDFLAGS="-L$with_sqlite/lib $LDFLAGS"
                                CPPFLAGS="-I$with_sqlite/include $CPPFLAGS"
                                AC_SEARCH_LIBS([sqlite3_open], [sqlite3],
                                [
                                        DBCPPFLAGS="$DBCPPFLAGS -I$with_sqlite/include"
                                        DBLDFLAGS="$DBLDFLAGS -L$with_sqlite/lib/ -lsqlite3"
                                ],[sqlite="no"],[-ldl])
                                LDFLAGS=$svd_LDFLAGS
                                CPPFLAGS=$svd_CPPFLAGS
   
                        ],
                        AC_SEARCH_LIBS([sqlite3_open], [sqlite3], [], [sqlite="no"], [-ldl]))
                fi
        ], 
        [
                AC_MSG_RESULT([yes])
                AC_SEARCH_LIBS([sqlite3_open], [sqlite3], [], [sqlite="no"])
        ])
if test "xyes" = "x$sqlite"; then
        AC_DEFINE([HAVE_LIBSQLITE3], 1, [Define to 1 to enable sqlite3])
        AC_SEARCH_LIBS([sqlite3_soft_heap_limit], [sqlite3], [AC_DEFINE([HAVE_SQLITE3_SOFT_HEAP_LIMIT], [1], [sqlite3_soft_heap_limit])], [], [-ldl])
        AC_SEARCH_LIBS([sqlite3_soft_heap_limit64], [sqlite3], [AC_DEFINE([HAVE_SQLITE3_SOFT_HEAP_LIMIT64], [1], [sqlite3_soft_heap_limit64])], [], [-ldl])
        AC_SEARCH_LIBS([sqlite3_errstr], [sqlite3], [AC_DEFINE([HAVE_SQLITE3_ERRSTR], [1], [sqlite3_errstr])], [], [-ldl])
fi
AM_CONDITIONAL([WITH_SQLITE], test "xyes" = "x$sqlite")

oracle="yes"
AC_MSG_CHECKING(for oracle)
AX_LIB_ORACLE_OCI
if test -n "$ORACLE_OCI_CFLAGS" -a -n "$ORACLE_OCI_LDFLAGS"; then
        DBCPPFLAGS="$DBCPPFLAGS $ORACLE_OCI_CFLAGS"
        DBLDFLAGS="$DBLDFLAGS $ORACLE_OCI_LDFLAGS"
        AC_DEFINE([HAVE_ORACLE], 1, [Define to 1 to enable oracle])
else
        oracle="no"
fi
AM_CONDITIONAL([WITH_ORACLE], test "xyes" = "x$oracle")

# Test if any database system was found
if test "xno" = "x$postgresql" -a "xno" = "x$mysql" -a "xno" = "x$sqlite" -a "xno" = "x$oracle"; then
        AC_MSG_ERROR([No available database found or selected. Try configure --help])
fi

AC_SUBST(DBLDFLAGS)
AC_SUBST(DBCPPFLAGS)

# ---------------------------------------------------------------------------
# Header files 
# ---------------------------------------------------------------------------

AC_HEADER_STDC

# ------------------------------------------------------------------------
# Functions 
# ------------------------------------------------------------------------

# Require a working setjmp
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
        #include <setjmp.h>
        ]],
        [[jmp_buf env; setjmp(env);]])], 
        [], [AC_MSG_FAILURE([setjmp is required])]
)

# Require that we have vsnprintf that conforms to c99. I.e. does bounds check
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
        #include <stdarg.h> 
        #include <stdio.h>
        ]],
        [[char t[1]; va_list ap; int n = vsnprintf(t, 1, "hello", ap); if(n == 5) return 0;return 1;]])], 
        [], [AC_MSG_FAILURE([vsnprintf does not conform to c99])]
)

AC_CHECK_FUNCS([timegm])


# ---------------------------------------------------------------------------
# Data Types
# ---------------------------------------------------------------------------

AC_CHECK_MEMBERS([struct tm.tm_gmtoff], [], [], [[#include <time.h>]])

# ---------------------------------------------------------------------------
# Compiler 
# ---------------------------------------------------------------------------

# Compiler characteristics
AC_C_CONST
AC_C_BIGENDIAN

# If the compiler is gcc, tune warnings and make the char type unsigned 
# and enable C99 support
if test "x$GCC" = "xyes"; then
        CFLAGS="$CFLAGS -Wall -Wunused -Wno-unused-label -funsigned-char";
        # Add C99 support, but with gnu extensions to include timegm and struct tm.tm_gmtoff
        CFLAGS="$CFLAGS -std=gnu99"
        # does this compiler support -Wno-pointer-sign ?
        svd_CFLAGS="$CFLAGS"
        CFLAGS="-Wno-pointer-sign $CFLAGS"
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [return 0;])], [], [CFLAGS="$svd_CFLAGS"])        
        # does this compiler support -Wno-address ?
        svd_CFLAGS="$CFLAGS"
        CFLAGS="-Wno-address $CFLAGS"
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [return 0;])], [], [CFLAGS="$svd_CFLAGS"])
fi

# ------------------------------------------------------------------------
# Architecture/OS
# ------------------------------------------------------------------------

case `uname` in
Linux*)   
	AC_DEFINE([LINUX], 1, [Define to 1 if the system is Linux])
	;;
FreeBSD*) 
	AC_DEFINE([FREEBSD], 1, [Define to 1 if the system is FreeBSD])
	;;
OpenBSD*) 
	AC_DEFINE([OPENBSD], 1, [Define to 1 if the system is OpenBSD])
	;;
Darwin*)  
	AC_DEFINE([DARWIN], 1, [Define to 1 if the system is OSX])
	;;
SunOS*)   
	AC_DEFINE([SOLARIS], 1, [Define to 1 if the system is SOLARIS])
	;;
NetBSD*)  
	AC_DEFINE([NETBSD], 1, [Define to 1 if the system is NETBSD])
	;;
AIX*) 	  
	AC_DEFINE([AIX], 1, [Define to 1 if the system is AIX])
	;;
esac


# ---------------------------------------------------------------------------
# Outputs
# ---------------------------------------------------------------------------

AC_CONFIG_HEADERS(src/xconfig.h)
AC_CONFIG_FILES([
        Makefile
        test/Makefile
        zdb.pc
])
AC_OUTPUT

cat <<EOT
+------------------------------------------------------------+
| License:                                                   |
| This is Open Source Software and use is subject to the GNU |
| GENERAL PUBLIC LICENSE, available in this distribution in  |
| the file COPYING.                                          |
|                                                            |
| By continuing this installation process, you are bound by  | 
| the terms of this license agreement. If you do not agree   |
| with the terms of this license, you must abort the         |
| installation process at this point.                        |
+------------------------------------------------------------+
| Libzdb is configured as follows:                           |
|                                                            |
EOT

if test "x1" = "x$OPTIMIZED"; then
echo "|   Optimized:                                    ENABLED    |"
else
echo "|   Optimized:                                    DISABLED   |"
fi
if test "xfalse" = "x$protect" -a "xfalse" = "x$zild_protect"; then
echo "|   Protected:                                    DISABLED   |"
else
echo "|   Protected:                                    ENABLED    |"
fi
if test "xfalse" = "x$profile"; then
echo "|   Profiling:                                    DISABLED   |"
else
echo "|   Profiling:                                    ENABLED    |"
fi
if test "xfalse" = "x$zild_protect"; then
echo "|   Zild:                                         DISABLED   |"
else
echo "|   Zild:                                         ENABLED    |"
fi
if test "x1" = "x$SQLITEUNLOCK"; then
echo "|   Sqlite3 unlock notification API:              ENABLED    |"
else
echo "|   Sqlite3 unlock notification API:              DISABLED   |"
fi
if test "xtrue" = "x$openssl"; then
echo "|   Openssl:                                      ENABLED    |"
else
echo "|   Openssl:                                      DISABLED   |"
fi
if test "xfalse" = "x$test_build"; then
echo "|   Unit Tests Build:                             DISABLED   |"
else
echo "|   Unit Tests Build:                             ENABLED    |"
fi
echo "|------------------------------------------------------------|"
if test "xyes" = "x$sqlite"; then
echo "|   SQLite3:                                      ENABLED    |"
else
echo "|   SQLite3:                                      DISABLED   |"
fi
if test "xyes" = "x$mysql"; then
echo "|   MySQL:                                        ENABLED    |"
else
echo "|   MySQL:                                        DISABLED   |"
fi
if test "xyes" = "x$postgresql"; then
echo "|   PostgreSQL:                                   ENABLED    |"
else
echo "|   PostgreSQL:                                   DISABLED   |"
fi
if test "xyes" = "x$oracle"; then
echo "|   Oracle:                                       ENABLED    |"
else
echo "|   Oracle:                                       DISABLED   |"
fi
echo "+------------------------------------------------------------+"


