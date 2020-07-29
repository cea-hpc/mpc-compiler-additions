check_PROGRAMS=nopriv_base_test
TESTS=$(check_PROGRAMS)
GLOB_COPT=-I$(top_srcdir)/tests/include -Wall -Werror -I$(top_srcdir)/libextls/src
GLOB_LOPT=-lextls -L$(top_builddir)/libextls/src

nopriv_base_test_CFLAGS=-I$(top_srcdir)/tests/include 
nopriv_base_test_LDFLAGS=
nopriv_base_test_SOURCES=nopriv_base.c

if HAVE_GETOPT

check_PROGRAMS+= getopt_test

getopt_test_CFLAGS=$(GLOB_COPT) -I$(top_builddir)/getopt/
getopt_test_LDFLAGS=-lpthread $(GLOB_LOPT)
getopt_test_SOURCES=getopt.c

endif