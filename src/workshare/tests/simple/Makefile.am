bin_PROGRAMS=simple simple2 test_stealing_collectives test_stealing_workers
CC=mpc_cc
WSFLAGS=-fws
OMPFLAGS=-fopenmp
simple_CFLAGS=-I$(top_srcdir)/tests/include $(WSFLAGS)
simple_LDFLAGS=
simple_SOURCES=simple.c

simple2_CFLAGS=-I$(top_srcdir)/tests/include $(WSFLAGS)
simple2_LDFLAGS=
simple2_SOURCES=simple2.c
-
test_stealing_collectives_CFLAGS=-I$(top_srcdir)/tests/include $(WSFLAGS)
test_stealing_collectives_LDFLAGS=
test_stealing_collectives_SOURCES=test_stealing_collectives.c

test_stealing_workers_CFLAGS=-I$(top_srcdir)/tests/include $(WSFLAGS) $(OMPFLAGS)
test_stealing_workers_LDFLAGS=
test_stealing_workers_SOURCES=test_stealing_workers.c


