check_PROGRAMS= \
	dynhls_test \
	noctx_test \
	$(NULL)


TESTS = $(check_PROGRAMS)

export MPC_DYN_PRIV_ENABLED=1
GLOB_COPT=-fmpc-privatize -I$(top_srcdir)/tests/include -Wall -Werror
GLOB_LOPT=-lextls

noctx_test_CFLAGS=$(GLOB_COPT)
noctx_test_LDFLAGS=$(GLOB_LOPT)
noctx_test_SOURCES=noctx.c

dynhls_test_CFLAGS=$(GLOB_COPT) -fhls -fhls-verbose
dynhls_test_LDFLAGS=$(GLOB_LOPT)
dynhls_test_SOURCES=hls_dyn.c
