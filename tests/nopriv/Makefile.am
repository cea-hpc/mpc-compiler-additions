check_PROGRAMS=nopriv_base_test
TESTS=$(check_PROGRAMS)
GLOB_COPT=-fmpc-privatize -I$(top_srcdir)/tests/include -Wall -Werror
GLOB_LOPT=-lextls

nopriv_base_test_CFLAGS=-I$(top_srcdir)/tests/include
nopriv_base_test_LDFLAGS=
nopriv_base_test_SOURCES=nopriv_base.c
