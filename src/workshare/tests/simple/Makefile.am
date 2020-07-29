check_PROGRAMS=simple_test another_test

TESTS=$(check_PROGRAMS)
GLOB_COPT=-O3

simple_test_CFLAGS=-I$(top_srcdir)/tests/include $(GLOB_COPT)
simple_test_LDFLAGS=
simple_test_SOURCES=simple.c

another_test_CFLAGS=-I$(top_srcdir)/tests/include $(GLOB_COPT)
another_test_LDFLAGS=
another_test_SOURCES=simple2.c

