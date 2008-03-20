AUTOMAKE_OPTIONS = foreign no-dependencies

LDADD = ../libzdb.la
INCLUDES = -I../src -I../src/util -I../src/net -I../src/db 

noinst_PROGRAMS = unit pool select
unit_SOURCES = unit.c
pool_SOURCES = pool.c
select_SOURCES = select.c

if WITH_EXCEPTIONS
INCLUDES += -I../src/exceptions
noinst_PROGRAMS += exception
exception_SOURCES = exception.c
endif

DISTCLEANFILES = *~ 

distclean-local: 
	-rm -f Makefile.in

test: unit pool select

verify:
	@/bin/sh @EXCEPTIONTEST@ ./unit && ./pool
