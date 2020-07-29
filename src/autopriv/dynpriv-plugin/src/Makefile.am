lib_LTLIBRARIES = dynpriv.la 
dynpriv_la_LDFLAGS=-module
dynpriv_la_CPPFLAGS= -I$(GCCDIR)/include

if OLDGCC
dynpriv_la_SOURCES = plugin_backward.cpp
else
dynpriv_la_SOURCES = plugin.cpp
endif
