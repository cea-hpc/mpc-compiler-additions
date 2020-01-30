##################
#### PREAMBLE ####
##################
AC_PREREQ([2.69])                                    # Minimum Autoconf Version
m4_define(AUTOPRIV_VERSION, m4_esyscmd_s([. $PWD/.autopriv_version]))
AC_INIT([AUTOPRIV], [AUTOPRIV_VERSION], [marc.perache@cea.fr])  # Init project configuration
AC_CONFIG_AUX_DIR([config-aux])                      # Autotools config files
AC_CONFIG_MACRO_DIR([config-aux/m4])                 # dependencies macros files
AM_INIT_AUTOMAKE([-Wall -Werror])
#AM_SILENT_RULES([yes])                              # Silent Make

##################
#### PROGRAMS ####
##################
AC_PROG_CC
AM_PROG_AR

###################
##### LIBTOOL #####
###################
LT_INIT                                              # Init libtool
AC_CONFIG_SUBDIRS([libextls dynpriv-plugin])
AC_CONFIG_HEADERS([config.h])                        # header-generated config files
AC_CONFIG_FILES([
		 Makefile
		 tests/Makefile
		 tests/nopriv/Makefile
		 tests/priv/Makefile
		 tools/Makefile
		 ])
AC_CONFIG_FILES([tools/autopriv.sh] , [chmod +x tools/autopriv.sh])
AC_CONFIG_FILES([tools/apcc]        , [chmod +x tools/apcc])
AC_CONFIG_FILES([tools/ap++]        , [chmod +x tools/ap++])
AC_CONFIG_FILES([tools/apfortran]   , [chmod +x tools/apfortran])

################
#### OUTPUT ####
################
AC_OUTPUT
