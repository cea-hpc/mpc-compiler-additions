#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([workshare-tests], [0.1], [marc.perache@cea.fr])
AC_CONFIG_SRCDIR([tests/include/ws_test.h])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([-Wall -Werror])


# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 tests/Makefile
                 tests/simple/Makefile
                 tests/fortran/Makefile])
AC_OUTPUT
