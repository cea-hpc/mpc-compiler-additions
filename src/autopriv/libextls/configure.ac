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
#### RPATH ######
#################

#
# Try to add a given PKGCONFIG pkg in rpath
#
# - $1 : package name
#
pkg_config_rpath()
{
		# Now atempt to extract HWLOC prefix
		pkg_rpath=""
		pkg_prefix=$(pkg-config --variable=prefix ${1})
		
		# Did we get something from pkg-config ?
		if test "x$?" = "x0"; then
			# See if there is a libdir
			if test -d "${pkg_prefix}/lib"; then
				pkg_rpath=" -Wl,-rpath=${pkg_prefix}/lib"
			fi
	
			# Try lib64 dir for existency
			if test -d "${pkg_prefix}/lib64"; then
				pkg_rpath="${HWLOC_RPATH} -Wl,-rpath=${pkg_prefix}/lib64"
			fi
		
			# Add to cflags if elements found
			if test -n "${pkg_rpath}"; then
				CFLAGS="${CFLAGS} ${pkg_rpath}"
			fi	
		fi
}


#################
##### HWLOC #####
#################
AC_ARG_WITH([hwloc], AS_HELP_STRING([--with-hwloc=PATH], [Provide hwloc installation path]))

if test -n "$with_hwloc" ; then
		if test ! -d $with_hwloc; then
			AC_MSG_ERROR([--with-hwloc path does not point to a directory])
		fi
		HWLOC_LDFLAGS="-lhwloc"
		LIBS="-L$with_hwloc/lib -L$with_hwloc/lib64 $LIBS"
		CFLAGS="-I$with_hwloc/include -Wl,-rpath=${with_hwloc}/lib64 -Wl,-rpath=${with_hwloc}/lib $CFLAGS"
		CPPFLAGS="-I$with_hwloc/include $CPPFLAGS"
else
	# First try with PKG config
	PKG_CHECK_MODULES([HWLOC], [hwloc], [pkg_found=1], [pkg_found=0])

	if test "x$pkg_found" = "x1"
	then
		pkg_config_rpath "hwloc"

		HWLOC_LDFLAGS="${HWLOC_LIBS}"
		CFLAGS="${HWLOC_CFLAGS} ${HWLOC_RPATH} $CFLAGS"
		CPPFLAGS="${HWLOC_CFLAGS} $CPPFLAGS"
	fi
fi

AC_SEARCH_LIBS([hwloc_topology_init], [hwloc], [HWLOC_OK=1], [HWLOC_OK=0])
AS_IF([test "x$HWLOC_OK" = "x1"],
				[
				AC_DEFINE([HAVE_HWLOC], [1], [Define hardware topology support])
										AC_SUBST(HWLOC_DEP, [hwloc])
				AC_SUBST(HWLOC_LDFLAGS)
				AC_SUBST(HWLOC_DEP, [hwloc])
				],
				[AC_MSG_WARN([Hwloc library not found ! - Disabling HLS])])


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
		pkg_config_rpath "openpa"
		
		LIBS="${OPENPA_LIBS} ${LIBS}"
		CFLAGS="${OPENPA_CFLAGS} $CFLAGS"
		CPPFLAGS="${OPENPA_CFLAGS} $CPPFLAGS"
	fi

fi

AC_CHECK_HEADER([opa_config.h], [OPA_OK=1], [OPA_OK=0])
AS_IF([test "x$OPA_OK" = "x1"],
				[AC_DEFINE([HAVE_ATOMICS], [1], [Define Atomic operation support])],
				[AC_MSG_WARN([Atomic Operation (OpenPA) library not found ! - Disabling HLS])])


##################
##### LibELF #####
##################

AC_ARG_WITH([libelf], AS_HELP_STRING([--with-libelf=PATH], [Provide libelf installation path]))

if test -n "$with_libelf" ; then
		if test ! -d $with_libelf; then
			AC_MSG_ERROR([--with-libelf path does not point to a directory])
		fi
		LIBELF_LDFLAGS="-lelf"
		LIBS="-L$with_libelf/lib -L$with_libelf/lib64 $LIBS"
		CFLAGS="-I$with_libelf/include $CFLAGS"
else

	# First try with PKG config
	PKG_CHECK_MODULES([LIBELF], [libelf], [pkg_found=1], [pkg_found=0])

	if test "x$pkg_found" = "x1"
	then
		pkg_config_rpath "libelf"
		
		LIBELF_LDFLAGS="${LIBELF_LIBS}"
		CFLAGS="${LIBELF_CFLAGS} $CFLAGS"
		CPPFLAGS="${LIBELF_CFLAGS} $CPPFLAGS"
	fi
fi

AC_SEARCH_LIBS([elf_version], [elf], [LIBELF_OK=1], [LIBELF_OK=0])
AS_IF([test "x$LIBELF_OK" = "x1"],
				[
				AC_DEFINE([HAVE_LIBELF], [1], [Define libelf support])
				AC_SUBST(LIBELF_LDFLAGS)
				AC_SUBST(LIBELF_DEP, [libelf])
				],
				[AC_MSG_WARN([Libelf library not found ! - Disabling optimized introspection])])

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
