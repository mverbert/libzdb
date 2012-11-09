AUTOMAKE_OPTIONS = foreign no-dependencies

LDADD = ../libzdb.la
AM_CPPFLAGS = -I../src -I../src/util -I../src/net -I../src/db -I../src/exceptions

noinst_PROGRAMS = unit pool select exception
unit_SOURCES = unit.c
pool_SOURCES = pool.c
select_SOURCES = select.c
exception_SOURCES = exception.c

DISTCLEANFILES = *~ 

distclean-local: 
	-rm -f Makefile.in

test: unit pool select

verify:
	@/bin/sh ./exception && ./unit && ./pool
