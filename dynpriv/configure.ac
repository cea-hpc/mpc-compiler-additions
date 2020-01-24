AC_PREREQ([2.64])
AC_INIT([GCC Automatic Privatization Plugin], [1.1.0], [jbbesnard@paratools.fr])
AC_CONFIG_SRCDIR([src/plugin.cpp])
AC_CONFIG_AUX_DIR([config-aux])
AC_CONFIG_MACRO_DIR([config-aux/m4])

###################################################################################
###################################################################################

AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_CONFIG_HEADERS([config.h])

# Checks
AM_PROG_AR
AC_PROG_CXX
AC_PROG_GREP
AC_C_INLINE
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADER_STDBOOL
AC_CHECK_FUNCS([strstr memset])

###################################################################################
###################################################################################

GCCDIR=`$CXX -print-file-name=plugin`
AC_SUBST(GCCDIR)
AM_CONDITIONAL(PROPER_GCC, [test -f "$GCCDIR/include/gcc-plugin.h"])
AM_COND_IF(PROPER_GCC, [AC_MSG_NOTICE(GCC supports plugins)], [AC_MSG_ERROR([GCC does not have plugin support. Please install it first])])

###################################################################################
###################################################################################

version_str="`$CXX --version`"
$CXX --version | $GREP -q "4\.8\..+"
if test "x$?" = "x0"; then
	AC_MSG_NOTICE([Old GCC compiler detected ! Using 4.x.x version.])
	OLDGCC_FOUND=yes
else
	CFLAGS+=" -fno-rtti"
	CXXFLAGS+=" -fno-rtti"
	AC_MSG_NOTICE([Recent GCC compiler detected.])
	OLDGCC_FOUND=
fi

AM_CONDITIONAL(OLDGCC, [test -n "$OLDGCC_FOUND"])

###################################################################################
###################################################################################

LT_INIT
AC_OUTPUT
