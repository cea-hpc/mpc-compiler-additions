check_PROGRAMS= \
	basic_test \
	kwd_test \
	dyn_test \
	hls_test \
	$(NULL)

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

dyn_test_CFLAGS=$(GLOB_COPT) -fplugin=$(top_builddir)/dynpriv-plugin/src/.libs/dynpriv.so
dyn_test_LDFLAGS=$(GLOB_LOPT)
dyn_test_SOURCES=dyn.c

hls_test_CFLAGS=$(GLOB_COPT) -fplugin=$(top_builddir)/dynpriv-plugin/src/.libs/dynpriv.so -fhls
hls_test_LDFLAGS=$(GLOB_LOPT)
hls_test_SOURCES=hls.c
