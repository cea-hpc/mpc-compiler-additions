##################
#### PREAMBLE ####
##################
AC_PREREQ([2.69])                                    # Minimum Autoconf Version
m4_define(EXTLS_VERSION, m4_esyscmd_s([. $PWD/.extls_version]))
AC_INIT([LIBEXTLS], [EXTLS_VERSION], [marc.perache@cea.fr])  # Init project configuration
AC_CONFIG_SRCDIR([src/extls.h])                      # Safety check for --srcdir
AC_CONFIG_AUX_DIR([config-aux])                      # Autotools config files
AC_CONFIG_MACRO_DIR([config-aux/m4])                 # dependencies macros files
AC_CONFIG_HEADERS([config.h])                        # header-generated config files
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
LT_LANG([C])
AC_PROG_LIBTOOL
AM_PROG_LIBTOOL

#########################
#### GENERATED FILES ####
#########################
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([doc/Makefile tools/Makefile tools/libextls.sh tools/extls.pc])

###################################################################################
###################################################################################

#################
#### DOXYGEN ####
#################
AC_ARG_WITH([doxygen], AS_HELP_STRING([--with-doxygen=PATH], [Provide custom Doxygen binary path for documentation]), [DOXYGEN_BIN=${withval}], [DOXYGEN_BIN=doxygen])

AC_CHECK_PROGS(DOXYGEN, [${DOXYGEN_BIN}])

AM_CONDITIONAL(HAVE_DOXYGEN, [test -n "${DOXYGEN}"])
AM_COND_IF([HAVE_DOXYGEN],
		   [AC_CONFIG_FILES([doc/Doxyfile])],
		   AC_MSG_WARN([Doxygen not found - continuing without Documentation support]))

#################
##### HWLOC #####
#################
AC_ARG_WITH([hwloc], AS_HELP_STRING([--with-hwloc=PATH], [Provide hwloc installation path]))

if test -n "$with_hwloc" ; then
		if test ! -d $with_hwloc; then
			AC_MSG_ERROR([--with-hwloc path does not point to a directory])
		fi
		LIBS="-L$with_hwloc/lib -L$with_hwloc/lib64 $LIBS"
		CFLAGS="-I$with_hwloc/include $CFLAGS"
		CPPFLAGS="-I$with_hwloc/include $CPPFLAGS"
else
	# First try with PKG config
	PKG_CHECK_MODULES([HWLOC], [hwloc], [pkg_found=1], [pkg_found=0])

	if test "x$pkg_found" = "x1"
	then
		AC_DEFINE([HAVE_HWLOC], [1], [Define hardware topology support])
		AC_SUBST(HWLOC_LDFLAGS)
		AC_SUBST(HWLOC_DEP, [hwloc])
		CFLAGS="-I${HWLOC_CFLAGS} $CFLAGS"
		CPPFLAGS="-I${HWLOC_CFLAGS} $CPPFLAGS"
	else

		AC_SEARCH_LIBS([hwloc_topology_init], [hwloc], [HWLOC_OK=1], [HWLOC_OK=0])
		AS_IF([test "x$HWLOC_OK" = "x1"],
						[
						AC_DEFINE([HAVE_HWLOC], [1], [Define hardware topology support])
						AC_SUBST(HWLOC_LDFLAGS, [-lhwloc])
						AC_SUBST(HWLOC_DEP, [hwloc])
						],
						[AC_MSG_WARN([Hwloc library not found ! - Disabling HLS])])

	fi
fi

##################
##### OpenPA #####
##################
AC_ARG_WITH([openpa], AS_HELP_STRING([--with-openpa=PATH], [Provide OpenPA installation path]))

if test -n "$with_openpa" ; then
		if test ! -d $with_openpa; then
			AC_MSG_ERROR([--with-openpa path does not point to a directory])
		fi
		LIBS="-L$with_openpa/lib -L$with_openpa/lib64 $LIBS"
		CFLAGS="-I$with_openpa/include $CFLAGS"
		CPPFLAGS="-I$with_openpa/include $CPPCFLAGS"
else

	# First try with PKG config
	PKG_CHECK_MODULES([OPENPA], [openpa], [pkg_found=1], [pkg_found=0])

	if test "x$pkg_found" = "x1"
	then
		AC_DEFINE([HAVE_ATOMICS], [1], [Define Atomic operation support])
		AC_SUBST(OPENPA_LDFLAGS)
		CFLAGS="-I${OPENPA_CFLAGS} $CFLAGS"
		CPPFLAGS="-I${OPENPA_CFLAGS} $CPPFLAGS"
	else
		AC_CHECK_HEADER([opa_config.h], [OPA_OK=1], [OPA_OK=0])
		AS_IF([test "x$OPA_OK" = "x1"],
						[AC_DEFINE([HAVE_ATOMICS], [1], [Define Atomic operation support])],
						[AC_MSG_WARN([Atomic Operation (OpenPA) library not found ! - Disabling HLS])])

	fi

fi

##################
##### LibELF #####
##################

AC_ARG_WITH([libelf], AS_HELP_STRING([--with-libelf=PATH], [Provide libelf installation path]))

	echo "######################## aaa! >$with_libelf<"
if test -n "$with_libelf" ; then
		if test ! -d $with_libelf; then
			AC_MSG_ERROR([--with-libelf path does not point to a directory])
		fi
		LIBS="-L$with_libelf/lib -L$with_libelf/lib64 $LIBS"
		CFLAGS="-I$with_libelf/include $CFLAGS"
else
	echo "######################## !"
	# First try with PKG config
	PKG_CHECK_MODULES([LIBELF], [libelf], [pkg_found=1], [pkg_found=0])

	if test "x$pkg_found" = "x1"
	then
		AC_DEFINE([HAVE_LIBELF], [1], [Define libelf support])
		AC_SUBST(LIBELF_DEP, [libelf])
		AC_SUBST(LIBELF_LDFLAGS)
		CFLAGS="-I${LIBELF_CFLAGS} $CFLAGS"
		CPPFLAGS="-I${LIBELF_CFLAGS} $CPPFLAGS"
	else
		AC_SEARCH_LIBS([elf_version], [elf], [LIBELF_OK=1], [LIBELF_OK=0])
		AS_IF([test "x$LIBELF_OK" = "x1"],
						[
						AC_DEFINE([HAVE_LIBELF], [1], [Define libelf support])
						AC_SUBST(LIBELF_LDFLAGS, [-lelf])
						AC_SUBST(LIBELF_DEP, [libelf])
						],
						[AC_MSG_WARN([Libelf library not found ! - Disabling optimized introspection])])
	fi
fi

##################
##### CUSTOM #####
##################
#AC_CHECK_PROGS(OBJREADER, readelf, [])
#AM_CONDITIONAL(HAVE_OBJREADER, [test -n "${OBJREADER}"])
#AM_COND_IF([HAVE_OBJREADER], [AC_SUBST(AM_CFLAGS, [-DHAVE_OBJREADER])], AC_MSG_WARN([Libextls does not support dynamic loading within user code without object file reader like readelf !]))
AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug], [Enable library debug mode (disabled by default)]))
AM_CONDITIONAL([DEBUG_MODE], [test "x$enable_debug" = "xyes"])

AC_ARG_ENABLE([color], AS_HELP_STRING([--enable-color], [Enable library color mode (disabled by default)]))
AS_IF([test "x$enable_color" = "xyes"], [
	   AC_DEFINE([EXTLS_COLOR_ENABLED], [1], [Shell color enabled])
		 ])
###################################################################################

###################################################################################

################
#### OUTPUT ####
################
AC_OUTPUT