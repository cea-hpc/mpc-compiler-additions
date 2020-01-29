check_PROGRAMS= \
	basic_test \
	kwd_test \
	dyn_test \
	noctx_test


TESTS = $(check_PROGRAMS)

####################
# PRIVATISATION TESTS
#####################
export MPC_DYN_PRIV_ENABLED=1
GLOB_COPT=-fmpc-privatize -I$(top_srcdir)/tests/include -Wall -Werror
GLOB_LOPT=-lextls

basic_test_CFLAGS=$(GLOB_COPT)
basic_test_LDFLAGS=$(GLOB_LOPT)
basic_test_SOURCES=basic.c

kwd_test_CFLAGS=$(GLOB_COPT)
kwd_test_LDFLAGS=$(GLOB_LOPT)
kwd_test_SOURCES=keywords.c

noctx_test_CFLAGS=$(GLOB_COPT)
noctx_test_LDFLAGS=$(GLOB_LOPT)
noctx_test_SOURCES=noctx.c

dyn_test_CPPFLAGS=$(GLOB_COPT) -fplugin=$(top_builddir)/dynpriv-plugin/src/.libs/dynpriv.so
dyn_test_LDFLAGS=$(GLOB_LOPT)
dyn_test_SOURCES=dyn.c
