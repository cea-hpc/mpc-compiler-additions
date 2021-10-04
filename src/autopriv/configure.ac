##################
#### PREAMBLE ####
##################
AC_PREREQ([2.69])                                    # Minimum Autoconf Version
m4_define(AUTOPRIV_VERSION, m4_esyscmd_s([. $PWD/../../.mpc_comp_additions_version]))
AC_INIT([AUTOPRIV], [AUTOPRIV_VERSION], [marc.perache@cea.fr])  # Init project configuration
AC_CONFIG_AUX_DIR([config-aux])                      # Autotools config files
AC_CONFIG_MACRO_DIR([config-aux/m4])                 # dependencies macros files
AM_INIT_AUTOMAKE([-Wall -Werror])
#AM_SILENT_RULES([yes])                              # Silent Make

##################
#### PROGRAMS ####
##################
AC_PROG_CC
AC_PROG_CXX
AC_PROG_FC
AM_PROG_AR

############################
###### COMPILER FAMILY #####
############################

EXTLS_COMPILER_FAMILY="notset"

extls_compiler_is_gcc="no"

AC_RUN_IFELSE([AC_LANG_PROGRAM([#include <stdlib.h>],[
#ifndef __GNUC__
exit(1);
#endif
])], [extls_compiler_is_gcc="yes"])

AC_MSG_CHECKING([if compiler is gcc])

if test "x${extls_compiler_is_gcc}" = "xyes"; then
        AC_MSG_RESULT([yes])
else
        AC_MSG_RESULT([no])
fi

extls_compiler_is_icc="no"

AC_RUN_IFELSE([AC_LANG_PROGRAM([#include <stdlib.h>],[
#ifndef __INTEL_COMPILER
exit(1);
#endif
])], [extls_compiler_is_icc="yes"])

AC_MSG_CHECKING([if compiler is icc])

if test "x${extls_compiler_is_icc}" = "xyes"; then
        AC_MSG_RESULT([yes])
else
        AC_MSG_RESULT([no])
fi

extls_compiler_is_clang="no"

AC_RUN_IFELSE([AC_LANG_PROGRAM([#include <stdlib.h>],[
#ifndef __clang__
exit(1);
#endif
])], [extls_compiler_is_clang="yes"])

AC_MSG_CHECKING([if compiler is clang])

if test "x${extls_compiler_is_clang}" = "xyes"; then
        AC_MSG_RESULT([yes])
else
        AC_MSG_RESULT([no])
fi

extls_compiler_is_pgi="no"

AC_RUN_IFELSE([AC_LANG_PROGRAM([#include <stdlib.h>],[
#ifndef __PGIC__
exit(1);
#endif
])], [extls_compiler_is_pgi="yes"])

AC_MSG_CHECKING([if compiler is pgi])

if test "x${extls_compiler_is_pgi}" = "xyes"; then
        AC_MSG_RESULT([yes])
else
        AC_MSG_RESULT([no])
fi


# Now set the compiler family

AC_MSG_CHECKING([compiler family to be])

if test "x$extls_compiler_is_icc" = "xyes"; then
        AC_MSG_RESULT([icc])
        EXTLS_COMPILER_FAMILY="icc"
else
        if test "x$extls_compiler_is_clang" = "xyes"; then
                AC_MSG_RESULT([clang])
                EXTLS_COMPILER_FAMILY="clang"
        else
                        if test "x$extls_compiler_is_gcc" = "xyes"; then
                                AC_MSG_RESULT([gcc])
                                EXTLS_COMPILER_FAMILY="gcc"
                        else
                                if test "x${extls_compiler_is_pgi}" = "xyes"; then
                                        EXTLS_COMPILER_FAMILY="pgi"
                                fi
                        fi
        fi

fi

AC_SUBST([EXTLS_COMPILER_FAMILY])

# Now determine Privatization flag

EXTLS_PRIV_FLAG=""

case $EXTLS_COMPILER_FAMILY in
        "gcc")
                EXTLS_PRIV_FLAG=" -fmpc-privatize"
        ;;
        "icc")
                EXTLS_PRIV_FLAG=" -fmpc-privatize"
        ;;
        "clang")
                EXTLS_PRIV_FLAG=" -fmpc-privatize"
        ;;
        "pgi")
                EXTLS_PRIV_FLAG=" -fmpc-privatize"
        ;;
        *)
                AC_MSG_ERROR([$CC ($EXTLS_COMPILER_FAMILY) is not supported for privatization])
        ;;
esac

AC_SUBST([EXTLS_PRIV_FLAG])

# And check if it is supported

PRIV_SUPPORT="no"

AX_CHECK_COMPILE_FLAG($EXTLS_PRIV_FLAG, [PRIV_SUPPORT="yes"])

###################
##### GETOPT  #####
###################

GETOPTINC="/usr/include/getopt.h"
if test ! -e $GETOPTINC; then
	AC_MSG_ERROR([Failed to locate getopt.h])
fi

AC_SUBST([GETOPTINC])

###################
##### LIBTOOL #####
###################
LT_INIT                                              # Init libtool

AC_ARG_ENABLE([extls], AS_HELP_STRING([--enable-extls], [Enable libextls library to be constructed (default)]), [enable_extls=$enableval], [enable_extls=yes])
if test "x$enable_extls" = "xyes"; then
	AC_CONFIG_SUBDIRS([libextls])
	AC_SUBST(EXTLS, [libextls])
fi


AUTOPRIV_GETOPT="$PRIV_SUPPORT"

AC_ARG_ENABLE([getopt], AS_HELP_STRING([--enable-getopt], [Enable privatized getopt library to be constructed (default)]), [enable_getopt=$enableval])
if test "x$enable_getopt" = "xyes"; then
	if test "x${PRIV_SUPPORT}" = "xno"; then
		AC_MSG_ERROR([Building privatized getopt requires a privatizing compiler])
	fi

	AUTOPRIV_GETOPT="getopt"

fi

AC_SUBST(AUTOPRIV_GETOPT)
AM_CONDITIONAL([HAVE_GETOPT], [test "x${AUTOPRIV_GETOPT}" = "xyes"])


AUTOPRIV_DYNPRIV=""

# If compiler family is not GCC dynpriv is disabled by default
if test "x${EXTLS_COMPILER_FAMILY}" != "xgcc" -o "x${PRIV_SUPPORT}" = "xno"; then
	enable_dynpriv="no"
else
	enable_dynpriv="yes"
fi

AC_ARG_ENABLE([dynpriv], AS_HELP_STRING([--enable-dynpriv], [Enable dynpriv GCC plugin to be constructed (default with GCC)]), [enable_dynpriv=$enableval], [])

if test "x$enable_dynpriv" = "xyes"; then
	if test ! "x${EXTLS_COMPILER_FAMILY}" = "xgcc"; then
		AC_MSG_ERROR([Dynamic privatization support is only supported with GCC])
	fi
	AC_CONFIG_SUBDIRS([dynpriv-plugin])
	AUTOPRIV_DYNPRIV="dynpriv-plugin"
fi

AC_SUBST(AUTOPRIV_DYNPRIV)


#AC_CONFIG_SUBDIRS([libextls dynpriv-plugin])
AC_CONFIG_HEADERS([config.h])                        # header-generated config files
AC_CONFIG_FILES([
		 Makefile
		 tests/Makefile
		 tests/nopriv/Makefile
		 tests/priv/Makefile
		 tests/unsupported/Makefile
		 tests/wrappers/Makefile
		 tools/Makefile
		 getopt/Makefile
		 getopt/getopt.h
		 tools/autopriv.pc
		 ])
AC_CONFIG_FILES([tools/autopriv.sh] , [chmod +x tools/autopriv.sh])
AC_CONFIG_FILES([tools/apcc]        , [chmod +x tools/apcc])
AC_CONFIG_FILES([tools/ap++]        , [chmod +x tools/ap++])
AC_CONFIG_FILES([tools/apfortran]   , [chmod +x tools/apfortran])

################
#### OUTPUT ####
################
AC_OUTPUT
