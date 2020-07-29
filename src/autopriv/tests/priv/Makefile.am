check_PROGRAMS= \
	basic_test \
	kwd_test \
	dyn_test \
	dyn_static_test \
	dyn_static_coll_test \
	hls_test \
	threads_test

TESTS = $(check_PROGRAMS)

####################
# PRIVATISATION TESTS
#####################
export MPC_DYN_PRIV_ENABLED=1
GLOB_COPT=-fmpc-privatize -I$(top_srcdir)/libextls/src -I$(top_srcdir)/tests/include -Wall -Werror
GLOB_LOPT=-L$(top_builddir)/libextls/src -lextls

basic_test_CFLAGS=$(GLOB_COPT)
basic_test_LDFLAGS=$(GLOB_LOPT)
basic_test_SOURCES=basic.c

kwd_test_CFLAGS=$(GLOB_COPT)
kwd_test_LDFLAGS=$(GLOB_LOPT)
kwd_test_SOURCES=keywords.c

dyn_test_CFLAGS=$(GLOB_COPT) -fplugin=$(top_builddir)/dynpriv-plugin/src/.libs/dynpriv.so
dyn_test_LDFLAGS=$(GLOB_LOPT)
dyn_test_SOURCES=dyn.c

dyn_static_test_CFLAGS=$(GLOB_COPT) -fplugin=$(top_builddir)/dynpriv-plugin/src/.libs/dynpriv.so
dyn_static_test_LDFLAGS=$(GLOB_LOPT)
dyn_static_test_SOURCES=dyn_static.c

dyn_static_coll_test_CFLAGS=$(GLOB_COPT) -fplugin=$(top_builddir)/dynpriv-plugin/src/.libs/dynpriv.so
dyn_static_coll_test_LDFLAGS=$(GLOB_LOPT)
dyn_static_coll_test_SOURCES=dyn_static_colla.c dyn_static_collb.c

hls_test_CFLAGS=$(GLOB_COPT) -fplugin=$(top_builddir)/dynpriv-plugin/src/.libs/dynpriv.so -fhls
hls_test_LDFLAGS=$(GLOB_LOPT)
hls_test_SOURCES=hls.c

threads_test_CFLAGS=$(GLOB_COPT)
threads_test_LDFLAGS=$(GLOB_LOPT) -lpthread
threads_test_SOURCES=threads.c

if HAVE_GETOPT

check_PROGRAMS += getopt_test

getopt_test_CFLAGS=$(GLOB_COPT) -I$(top_builddir)/getopt/ -DAUTOPRIV_ENABLED
getopt_test_LDFLAGS=$(GLOB_LOPT) -lpthread -L$(top_builddir)/getopt/ -lextlsgetopt 
getopt_test_SOURCES=getopt.c

endif
