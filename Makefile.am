# Copyright (C) Tildeslash Ltd. All rights reserved.

AUTOMAKE_OPTIONS = foreign no-dependencies subdir-objects
ACLOCAL_AMFLAGS  = -I m4

SUBDIRS         = . $(UNIT_TEST)
DIST_SUBDIRS    = . test

EXTRA_DIST      = README AUTHORS CHANGES COPYING bootstrap doc test src tools config

LIBRARY_NAME    = zdb

RE2C		= @RE2C@
RE2CFLAGS       = -b
FILTERH         = ./tools/bin/filterh

INCLUDES     = -Isrc -Isrc/util -Isrc/net -Isrc/db -Isrc/exceptions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= $(LIBRARY_NAME).pc

lib_LTLIBRARIES = libzdb.la
libzdb_la_SOURCES = src/util/Str.c src/util/Vector.c src/util/StringBuffer.c \
                    src/system/Mem.c src/system/System.c src/system/Time.c \
                    src/db/ConnectionPool.c src/db/Connection.c src/db/ResultSet.c \
                    src/db/PreparedStatement.c  \
                    src/exceptions/assert.c src/exceptions/Exception.c

if ! WITH_ZILD
libzdb_la_SOURCES += src/net/URL.c 
endif

LIBRARY_MAJOR_VERSION=13
libzdb_la_LDFLAGS = -version-info $(LIBRARY_MAJOR_VERSION):0:0
MODULE_NOVERSION= -module -avoid-version

if ! WITH_LIBDL
noinst_LTLIBRARIES =
libzdb_la_LIBADD = 
else
libzdb_la_LDFLAGS += -ldl
modules_LTLIBRARIES =
modulesdir=$(libdir)/@PACKAGE_NAME@$(LIBRARY_MAJOR_VERSION)
libzdb_la_CPPFLAGS=-DMODULESDIR="\"$(modulesdir)\""
endif

if WITH_MYSQL
mysql_la_SOURCES = src/db/mysql/MysqlConnection.c \
		          src/db/mysql/MysqlResultSet.c \
		          src/db/mysql/MysqlPreparedStatement.c
mysql_la_CPPFLAGS = $(DB_MYSQL_CPPFLAGS)
mysql_la_LDFLAGS = $(DB_MYSQL_LDFLAGS) $(MODULE_NOVERSION)

if WITH_LIBDL
modules_LTLIBRARIES += mysql.la
else
noinst_LTLIBRARIES += mysql.la
libzdb_la_LIBADD += mysql.la
endif
endif # WITH_MYSQL

if WITH_POSTGRESQL
postgresql_la_SOURCES = src/db/postgresql/PostgresqlConnection.c \
		               src/db/postgresql/PostgresqlResultSet.c \
		               src/db/postgresql/PostgresqlPreparedStatement.c
postgresql_la_CPPFLAGS = $(DB_POSTGRESQL_CPPFLAGS)
postgresql_la_LDFLAGS = $(DB_POSTGRESQL_LDFLAGS) $(MODULE_NOVERSION)

if WITH_LIBDL
modules_LTLIBRARIES += postgresql.la
else
noinst_LTLIBRARIES += postgresql.la
libzdb_la_LIBADD += postgresql.la
endif
endif # WITH_POSTGRESQL

if WITH_SQLITE
sqlite_la_SOURCES = src/db/sqlite/SQLiteConnection.c \
		    src/db/sqlite/SQLiteResultSet.c \
                    src/db/sqlite/SQLitePreparedStatement.c \
                    src/db/sqlite/SQLiteAdapter.c
sqlite_la_CPPFLAGS = $(DB_SQLITE_CPPFLAGS)
sqlite_la_LDFLAGS = $(DB_SQLITE_LDFLAGS) $(MODULE_NOVERSION) 

if WITH_LIBDL
modules_LTLIBRARIES += sqlite.la
else
noinst_LTLIBRARIES += sqlite.la
libzdb_la_LIBADD += sqlite.la
endif
endif # WITH_SQLITE

if WITH_ORACLE
oracle_la_SOURCES = src/db/oracle/OracleConnection.c \
	            src/db/oracle/OracleResultSet.c \
	            src/db/oracle/OraclePreparedStatement.c \
                    src/db/oracle/OracleAdapter.c

oracle_la_CPPFLAGS = $(DB_ORACLE_CPPFLAGS) -Isrc/db/oracle 
oracle_la_LDFLAGS = $(DB_ORACLE_LDFLAGS) $(MODULE_NOVERSION)

if WITH_LIBDL
modules_LTLIBRARIES += oracle.la
else
noinst_LTLIBRARIES += oracle.la
libzdb_la_LIBADD += oracle.la
endif
endif # WITH_ORACLE

API_INTERFACES  = src/zdb.h src/zdbpp.h src/db/ConnectionPool.h \
                  src/db/Connection.h src/db/ResultSet.h src/net/URL.h \
                  src/db/PreparedStatement.h src/exceptions/SQLException.h \
                  src/exceptions/Exception.h

nobase_nodist_include_HEADERS = $(patsubst %, $(LIBRARY_NAME)/%, $(notdir $(API_INTERFACES)))

BUILT_SOURCES   = $(nobase_nodist_include_HEADERS)

CLEANFILES      = $(BUILT_SOURCES)

DISTCLEANFILES  = *~ 

dist-hook::
	-rm -rf `find $(distdir) -name ".git"`
	-rm -rf `find $(distdir) -name "._*"`
	-rm -rf `find $(distdir) -name ".DS_Store"`
	-rm -rf `find $(distdir) -name ".libs"`
	-rm -rf `find $(distdir) -name ".svn"`
	-rm -f $(distdir)/src/xconfig.h $(distdir)/src/stamp-* \
              $(distdir)/tools/bin/filterh
        
clean-local:
		-rm -f `find src -name "*.o" -o -name "*.lo" -o -name "*.loT" \
                -o -name "*~" -o -name ".#*" -o -name "core*"`

distclean-local::
	-rm -rf autom4te.cache/ \
                build/ \
                $(LIBRARY_NAME)
	-rm -f  doc/api-docs/*h.html \
                doc/api-docs/doxygen.css \
                doc/api-docs/tabs.css \
                doc/api-docs/*.gif \
                tools/bin/filterh \
                src/xconfig.h.in \
                config/config.guess \
                config/config.sub

                        
cleanall: clean distclean
	-rm -f Makefile.in test/Makefile.in configure aclocal.m4 \
        libzdb-[0-9].*tar.gz conf/config.* $(pkgconfig_DATA)
	git clean -fxd

verify: libzdb.la
	cd $(srcdir)/test && $(MAKE) verify	

doc: $(nobase_nodist_include_HEADERS)
	doxygen config/Doxyfile
	-cp doc/api-docs/files.html doc/api-docs/index.html

define check-exit
|| exit 1

endef

$(nobase_nodist_include_HEADERS): $(API_INTERFACES)
	$(shell test -d $(LIBRARY_NAME) || mkdir $(LIBRARY_NAME))
	$(foreach file, $(API_INTERFACES), \
                $(FILTERH) < $(file) > $(LIBRARY_NAME)/$(notdir $(file)) \
	$(check-exit))

#<< start-filter out
distclean-local::
	-rm -f  src/net/URL.c \
                src/system/Time.c

src/net/URL.c: src/net/URL.re
	$(RE2C) $(RE2CFLAGS) $< > $@
src/system/Time.c: src/system/Time.re
	$(RE2C) $(RE2CFLAGS) $< > $@

dist-hook::
	-rm -f $(distdir)/src/net/URL.re
	-rm -f $(distdir)/src/system/Time.re
	$(FILTERH) < Makefile.am > $(distdir)/Makefile.am
	$(FILTERH) < Makefile.in > $(distdir)/Makefile.in
#>> end-filter out


