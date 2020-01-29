CFILES=\
	   extls.c\
	   extls_optim_tls.c\
	   extls_common.c\
	   extls_hls.c\
	   extls_mpc.c\
	   extls_dynamic.c\
       extls_segmt_hdler.c

HFILES=\
	   extls.h\
	   extls_locks.h\
	   extls_topo.h\
	   extls_atomics.h\
	   extls_common.h\
	   extls_optim_tls.h\
	   extls_hls.h\
	   extls_list.h\
	   extls_mpc.h\
	   extls_dynamic.h\
	   extls_segmt_hdler.h

lib_LTLIBRARIES= libextls.la
libextls_la_SOURCES= $(CFILES)
if DEBUG_MODE
libextls_la_CFLAGS=-Wall -Wextra -Werror -g -O0
else
libextls_la_CFLAGS=-DNDEBUG
endif
libextls_la_LDFLAGS = -ldl $(OPA_LDFLAGS) -lpthread $(HWLOC_LDFLAGS) $(LIBELF_LDFLAGS)
include_HEADERS=$(HFILES)
