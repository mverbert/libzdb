AUTOMAKE_OPTIONS = foreign no-dependencies

SUBDIRS         = . $(UNIT_TEST)
DIST_SUBDIRS    = . test

EXTRA_DIST      = README CHANGES AUTHORS COPYING EXCEPTIONS bootstrap \
                  doc test src tools config

RE2C		= @RE2C@
RE2CFLAGS       = -b
FILTERH         = ./tools/bin/filterh

AM_CPPFLAGS     = $(CPPFLAGS) $(DBCPPFLAGS)
INCLUDES        = -Isrc -Isrc/util -Isrc/net -Isrc/db -Isrc/exceptions

lib_LTLIBRARIES = libzdb.la
libzdb_la_SOURCES = src/util/Util.c src/util/Str.c src/util/Mem.c \
                    src/util/Vector.c src/net/URL.c src/db/ConnectionPool.c \
                    src/db/Connection.c src/db/ResultSet.c \
                    src/db/PreparedStatement.c src/util/StringBuffer.c \
                    src/exceptions/assert.c src/exceptions/Exception.c
if WITH_MYSQL
libzdb_la_SOURCES += src/db/mysql/MysqlConnection.c \
                     src/db/mysql/MysqlResultSet.c \
                     src/db/mysql/MysqlPreparedStatement.c
endif
if WITH_POSTGRESQL
libzdb_la_SOURCES += src/db/postgresql/PostgresqlConnection.c \
                     src/db/postgresql/PostgresqlResultSet.c \
                     src/db/postgresql/PostgresqlPreparedStatement.c
endif
if WITH_SQLITE
libzdb_la_SOURCES += src/db/sqlite/SQLiteConnection.c \
                     src/db/sqlite/SQLiteResultSet.c \
                     src/db/sqlite/SQLitePreparedStatement.c
endif

nodist_include_HEADERS = api/ConnectionPool.h api/Connection.h \
                         api/ResultSet.h api/URL.h api/PreparedStatement.h \
                         api/SQLException.h api/Exception.h

API_INTERFACES  = src/db/ConnectionPool.h src/db/Connection.h \
                  src/db/ResultSet.h src/net/URL.h src/db/PreparedStatement.h \
                  src/exceptions/SQLException.h src/exceptions/Exception.h

libzdb_la_LDFLAGS = $(DBLDFLAGS) -release @VERSION@ -version-info 6:0:1

BUILT_SOURCES   = $(nodist_include_HEADERS)

CLEANFILES      = $(BUILT_SOURCES)

DISTCLEANFILES  = *~ 

dist-hook::
	-rm -rf `find $(distdir) -name CVS`
	-rm -rf `find $(distdir) -name "._*"`
	-rm -rf `find $(distdir) -name ".DS_Store"`
	-rm -rf `find $(distdir) -name ".libs"`
	-rm -rf `find $(distdir) -name ".svn"`
	-rm -f $(distdir)/src/xconfig.h $(distdir)/src/stamp-* \
              $(distdir)/tools/bin/filterh
        
clean-local:
		-rm -f `find src -name "*.o" -o -name "*.lo" -o -name "*.loT" \
                -o -name "*~" -o -name ".#*" -o -name "core*"`

distclean-local: 
		-rm -rf autom4te.cache/ \
                        build/ \
                        api
		-rm -f  doc/api-docs/*h.html \
                        doc/api-docs/*.css \
                        doc/api-docs/*.gif \
                        tools/bin/filterh \
                        src/xconfig.h.in \
                        config/config.guess \
                        config/config.sub
                        
cleanall: clean distclean
	-rm -f Makefile.in test/Makefile.in configure aclocal.m4 \
        libzdb-[0-9].*tar.gz conf/config.*

verify: libzdb.la
	cd $(srcdir)/test && $(MAKE) verify	

doc: $(nodist_include_HEADERS)
	doxygen config/Doxyfile
	-cp doc/api-docs/files.html doc/api-docs/index.html

define check-exit
|| exit 1

endef

$(nodist_include_HEADERS): $(API_INTERFACES)
	$(shell test -d api || mkdir api)
	$(foreach file, $(API_INTERFACES), \
                $(FILTERH) < $(file) > api/$(notdir $(file)) \
	$(check-exit))

#<< start-filter out
src/net/URL.c: src/net/URL.re
	$(RE2C) $(RE2CFLAGS) $< > $@
        
dist-hook::
	-rm -rf $(distdir)/src/net/URL.re
	$(FILTERH) < Makefile.am > $(distdir)/Makefile.am
	$(FILTERH) < Makefile.in > $(distdir)/Makefile.in
#>> end-filter out


