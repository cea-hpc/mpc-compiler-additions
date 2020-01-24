AM_CPPFLAGS= -I$(GCCDIR)/include

lib_LTLIBRARIES = libdynpriv.la 

if OLDGCC
libdynpriv_la_SOURCES = plugin_backward.cpp
else
libdynpriv_la_SOURCES = plugin.cpp
endif
