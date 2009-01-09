# Copyright (C) 2004-2009 Tildeslash Ltd. All rights reserved.

AC_PREREQ([2.53])

AC_INIT([libzdb], [2.4], [bugs-libzdb@tildeslash.com])
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/Config.h])

# ---------------------------------------------------------------------------
# Programs 
# ---------------------------------------------------------------------------

AC_PROG_CC
AC_PROG_GCC_TRADITIONAL
AC_PATH_PROG([RE2C], [re2c], [no], [$PATH:/usr/local/bin:/usr/bin])
AC_PATH_PROG([FLEX], [flex], [no])
if test "x$FLEX" = "xno"; then
        AC_MSG_ERROR([flex is required. Download from http://www.gnu.org/software/flex/])
else
        MYCC=$CC
        MYFLEX=$FLEX
        export MYCC
        export MYFLEX
        if test ! -d ./tools/bin/
        then
                mkdir ./tools/bin || AC_MSG_ERROR([Failed to create ./tools/bin])
        fi
        /bin/sh ./tools/filterh/compfilterh || AC_MSG_ERROR([Failed to build tools])
fi

# ---------------------------------------------------------------------------
# Libtool 
# ---------------------------------------------------------------------------

AC_PROG_LIBTOOL

# ---------------------------------------------------------------------------
# Build options
# ---------------------------------------------------------------------------

AC_ARG_ENABLE(optimized,
	AS_HELP_STRING([--enable-optimized], 
        	[Build software optimized]),
    [
        CFLAGS=`echo $CFLAGS|sed 's/-O.//'`
        if test "x$enableval" = "xyes" ; then
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
                [Reduce visbility of objects for linking with zild. 
                Require gcc version 4.x]),
     [
                if test "x$GCC" = "xyes"; then
                        zild_protect="true"
                        AC_DEFINE([PACKAGE_PROTECTED], 1, 
                                [Define to 1 to package protect (hide) non-api objects])
                        AC_DEFINE([ZILD_PACKAGE_PROTECTED], 1, 
                                [Define to 1 to hide objects for linking with zild])
                else
                        AC_MSG_ERROR([This option (--enable-zild) require GCC])   
                fi
     ],[zild_protect="false"])

AC_ARG_ENABLE([protected],
        AS_HELP_STRING([--enable-protected], 
        [Package protect non-api objects. Require gcc version 4.x
         This option hide objects in the DSO which are not part of the api and
         not to be exported. The rationale is to optimize the ABI and protect
         non-public methods of the DSO and thereby reduce the potential for 
         namespace conflicts for clients linking with the library. Recommend]),
     [
        if test "x$GCC" = "xyes"; then
                protect="true"
                AC_DEFINE([PACKAGE_PROTECTED], 1, 
                        [Define to 1 to package protect (hide) non-api objects])
        else
             AC_MSG_ERROR([This option (--enable-protected) require GCC])   
        fi
     ],[protect="false"])

AC_ARG_ENABLE(profiling,
        AS_HELP_STRING([--enable-profiling],
                [Build with debug and profiling options]),
    [
        if test "x$enableval" = "xyes" ; then
                CFLAGS="$CFLAGS -g -pg"
                profile="true"
        fi
    ],
    [
        profile="false"
    ]
)

AC_ARG_ENABLE([openssl],
        AS_HELP_STRING([--enable-openssl(=<path>)], 
                [Link libzdb with openssl. If database libraries were linked static,
                libzdb may have to link with openssl to support crypto and ssl 
                functionality in SQL client libraries. An optional path argument may
                be given to specify the top-level directory to search for openssl to
                link with]),
     [
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
     ],[openssl="false"])

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

AC_CHECK_LIB([pthread], [pthread_create], [],
	[AC_MSG_ERROR([POSIX thread library is required])])

# Database Libraries 

mysql="yes"
AC_MSG_CHECKING(for mysql)
AC_ARG_WITH([mysql], 
        AS_HELP_STRING([--with-mysql=<path>], 
                [prefix of mysql installation. e.g. /usr/local/mysql]), 
        [
        if test "xno" = "x$with_mysql"; then
                AC_MSG_RESULT([mysql=no])
                mysql="no"
        else
                svd_LDFLAGS=$LDFLAGS
                svd_CPPFLAGS=$CPPFLAGS
                LDFLAGS="-L$with_mysql/lib $LDFLAGS"
                CPPFLAGS="-I$with_mysql/include $CPPFLAGS"
                if test -r $with_mysql/lib/libmysqlclient.*a; then
                        mysql="yes"
                        DBCPPFLAGS="$DBCPPFLAGS -I$with_mysql/include"
                        DBLDFLAGS="$DBLDFLAGS -L$with_mysql/lib/ -lmysqlclient"
                        AC_MSG_RESULT([ok, using libmysqlclient in $with_mysql/lib/])
                else
                        AC_MSG_RESULT([fail, cannot find libmysqlclient in $with_mysql/lib])
                        mysql="no"
                fi
                if test "xno" = "x$mysql"; then
                        LDFLAGS=$svd_LDFLAGS
                        CPPFLAGS=$svd_CPPFLAGS
                fi
        fi
        ], [AC_CHECK_LIB([mysqlclient], [mysql_init], [], [mysql="no"], [-lz])])

if test "xyes" = "x$mysql"; then
        AC_MSG_CHECKING(mysql version)
        CPPFLAGS="-I/usr/include/mysql $CPPFLAGS"
        AC_CHECK_HEADERS([mysql.h])
        AC_RUN_IFELSE([AC_LANG_PROGRAM([[#include <mysql.h>]],
        [[
        if (MYSQL_VERSION_ID < 40100)
                return(1);
        else
                return(0);
        ]])
        ],[mysql="yes"],[mysql="no"])
        if test "$mysql" = "no"
        then
                AC_MSG_RESULT([fail, >= 4.1 required])
        else
                AC_CHECK_LIB([z], [compress],[],[mysql="no"])
        fi
fi
if test "xyes" = "x$mysql"; then
        AC_DEFINE([HAVE_LIBMYSQLCLIENT])
fi
AM_CONDITIONAL([WITH_MYSQL], test "xyes" = "x$mysql")

postgresql="yes"
AC_MSG_CHECKING(for postgresql)
AC_ARG_WITH([postgresql], 
        AS_HELP_STRING([--with-postgresql=<path>], 
                [path to the pg_config e.g. /usr/local/pgsql/bin/pg_config]), 
        [
                if test "xno" = "x$with_postgresql"; then
                        AC_MSG_RESULT([no])
                        postgresql="no"
                else
                        if test "x" = "x$with_postgresql"; then
                                PGCONFIG="pg_config"
                        else
                                PGCONFIG="$with_postgresql"
                        fi
                fi
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
                AC_MSG_RESULT([no])
        fi
fi
AM_CONDITIONAL([WITH_POSTGRESQL], test "xyes" = "x$postgresql")

sqlite="yes"
AC_MSG_CHECKING(for SQLite3)
AC_ARG_WITH([sqlite], 
        AS_HELP_STRING([--with-sqlite=<path>], 
                [prefix of SQLite installation. e.g. /usr/local/sqlite3 or /usr]), 
        [
        if test "xno" = "x$with_sqlite"; then
                AC_MSG_RESULT([sqlite=no])
                sqlite="no"
        else
                svd_LDFLAGS=$LDFLAGS
                svd_CPPFLAGS=$CPPFLAGS
                LDFLAGS="-L$with_sqlite/lib $LDFLAGS"
                CPPFLAGS="-I$with_sqlite/include $CPPFLAGS"
                AC_CHECK_HEADERS([sqlite3.h], [
                        sqlite="yes"
                        if test -r "$with_sqlite/lib/libsqlite3.a"; then
                                DBCPPFLAGS="$DBCPPFLAGS -I$with_sqlite/include"
                                DBLDFLAGS="$DBLDFLAGS -L$with_sqlite/lib/ -lsqlite3"
                        else
                                sqlite="no"
                        fi
                ], [sqlite="no"])
                LDFLAGS=$svd_LDFLAGS
                CPPFLAGS=$svd_CPPFLAGS
        fi
        ], [AC_CHECK_LIB([sqlite3], [sqlite3_open], [], [sqlite="no"])])
if test "xyes" = "x$sqlite"; then
        AC_DEFINE([HAVE_LIBSQLITE3])
fi
AM_CONDITIONAL([WITH_SQLITE], test "xyes" = "x$sqlite")

if test "xno" = "x$postgresql" -a "xno" = "x$mysql" -a "xno" = "x$sqlite"; then
        AC_MSG_ERROR([No available database found or selected. Try configure --help])
fi

AC_SUBST(DBLDFLAGS)
AC_SUBST(DBCPPFLAGS)

# ---------------------------------------------------------------------------
# Header files 
# ---------------------------------------------------------------------------

AC_HEADER_STDC

# ---------------------------------------------------------------------------
# Compiler 
# ---------------------------------------------------------------------------

# Compiler characteristics
AC_C_CONST
AC_C_BIGENDIAN

# If the compiler is gcc, tune warnings and make the char type unsigned
if test "x$GCC" = "xyes"; then
	CFLAGS="$CFLAGS -Wall -Wunused -Wno-unused-label -funsigned-char";
        # does this compiler support -Wno-pointer-sign ?
        svd_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS -Wno-pointer-sign"
        AC_COMPILE_IFELSE([int main(){return 0;}], [], [CFLAGS="$svd_CFLAGS"])        
fi

# ---------------------------------------------------------------------------
# Functions 
# ---------------------------------------------------------------------------


# ---------------------------------------------------------------------------
# Outputs
# ---------------------------------------------------------------------------

AM_CONFIG_HEADER(src/xconfig.h)
AC_OUTPUT(Makefile test/Makefile)

cat <<EOT
+------------------------------------------------------------+
| License:                                                   |
| This is Open Source Software and use is subject to the GNU |
| GENERAL PUBLIC LICENSE with exceptions, available in this  |
| distribution in the files COPYING and EXCEPTIONS.          |
|                                                            |
| By continuing this installation process, you are bound by  | 
| the terms of this license agreement. If you do not agree   |
| with the terms of this license, you must abort the         |
| installation process at this point.                        |
+------------------------------------------------------------+
| The Zild Database Library is configured as follows:        |
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
if test "xfalse" = "x$test_build"; then
echo "|   Unit Tests Build:                             DISABLED   |"
else
echo "|   Unit Tests Build:                             ENABLED    |"
fi
if test "xtrue" = "x$openssl"; then
echo "|   Openssl:                                      ENABLED    |"
fi
echo "|------------------------------------------------------------|"
if test "xyes" = "x$sqlite"; then
echo "|   SQLite3:                                      ENABLED    |"
else
echo "|   SQLite3:                                      DISABLED   |"
fi
if test "xyes" = "x$mysql"; then
echo "|   Mysql:                                        ENABLED    |"
else
echo "|   Mysql:                                        DISABLED   |"
fi
if test "xyes" = "x$postgresql"; then
echo "|   PostgreSQL:                                   ENABLED    |"
else
echo "|   PostgreSQL:                                   DISABLED   |"
fi
echo "+------------------------------------------------------------+"


