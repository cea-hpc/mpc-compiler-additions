AM_CPPFLAGS= -I$(GCCPLUGINS_DIR)/include -I$(GCCPLUGINS_DIR)

lib_LTLIBRARIES = libprivplug.la 

if IS_48SERIES
libprivplug_la_SOURCES = plugin4.cpp
else
if IS_62SERIES
libprivplug_la_SOURCES = plugin6.cpp
else 
if IS_72SERIES
libprivplug_la_SOURCES = plugin7.cpp
endif
endif
endif
