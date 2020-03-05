AUTOMAKE_OPTIONS = foreign no-dependencies

DEFAULT_INCLUDES =
LDADD = ../libzdb.la
zdbpp_CXXFLAGS = -I../zdb -std=c++17
CFLAGS = -I../src -I../src/util -I../src/net -I../src/db -I../src/exceptions @CFLAGS@

noinst_PROGRAMS = unit pool select exception zdbpp
unit_SOURCES = unit.c
pool_SOURCES = pool.c
select_SOURCES = select.c
exception_SOURCES = exception.c
zdbpp_SOURCES = zdbpp.cpp

DISTCLEANFILES = *~

distclean-local: 
	-rm -f Makefile.in

test: unit pool select zdbpp

verify:
	@/bin/sh ./exception && ./unit && ./pool && ./zdbpp
