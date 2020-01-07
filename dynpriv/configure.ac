AC_PREREQ([2.64])
AC_INIT([GCC Privatization Plugin], [1.1], [jbbesnard@paratools.fr])
AC_CONFIG_SRCDIR([plugin4.cpp])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
AC_CONFIG_FILES([Makefile])
AM_SILENT_RULES([yes])

LT_INIT

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE

# Checks for library functions.
AC_CHECK_FUNCS([strstr])

# Check that target GCC supports plugins

GCCPLUGINS_DIR=`$CXX -print-file-name=plugin`
AC_SUBST([GCCPLUGINS_DIR])

AC_MSG_NOTICE([Target GCC plugin DIR is $GCCPLUGINS_DIR])

for f in gcc-plugin.h tree-pass.h basic-block.h gimple.h ipa-prop.h\
         tree-iterator.h gimple-pretty-print.h rtl.h \
         ggc.h hashtab.h tree-pass.h intl.h params.h timevar.h\
         diagnostic.h cgraph.h flags.h langhooks.h tree-inline.h\
         tree.h tm.h coretypes.h system.h
do

	if test -e "$GCCPLUGINS_DIR/include/${f}"; then
		AC_MSG_NOTICE([GCC Header ${f}... OK])
	else
		AC_MSG_ERROR([GCC Header ${f}... not found in $GCCPLUGINS_DIR])
	fi

done

CFLAGS+=" -I $PREFIX/include/"
CXXFLAGS+=" -I $PREFIX/include/"
	

AC_MSG_NOTICE([ALL plugin headers found ... OK])

$CXX -show > /dev/null 2>&1
if test "$?" = "0"; then
	AC_MSG_ERROR([Please provide a path to the privatized compiler, not the wrapper ($CXX)])
fi

AC_MSG_NOTICE([Compiler does not seem to be an MPC wrapper... ${MPC_CXX_COMPILER}])
MPC_CXX_COMPILER=${CXX}

cat << . > conftest.c
int main(){}
.

$MPC_CXX_COMPILER -fmpc-privatize conftest.c > /dev/null
RET=$?
rm -f ./conftest.c ./a.out

if test ${RET} != 0; then
	AC_MSG_ERROR([This does not appear to be a patched compiler ($MPC_CXX_COMPILER)])
fi

AC_MSG_NOTICE([Patched compiler found... ${MPC_CXX_COMPILER}])
AC_SUBST([MPC_CXX_COMPILER])

IS_48SERIES=0
IS_62SERIES=0
IS_72SERIES=0
IS_73SERIES=0

version_str="`$CXX --version`"

echo "$version_str" | egrep -o "4\.8\..+" > /dev/null
if test "x$?" = "x0"; then
	AC_MSG_NOTICE([GCC 4.8.x compiler detected... ${MPC_CXX_COMPILER}])
	IS_48SERIES=1
        AC_DEFINE([IS_48SERIES])
else
	CFLAGS+=" -fno-rtti"
	CXXFLAGS+=" -fno-rtti"
	
	echo "$version_str" | egrep -o "6\.2\..+" > /dev/null
	if test "x$?" = "x0"; then
		AC_MSG_NOTICE([GCC 6.2.x compiler detected... ${MPC_CXX_COMPILER}])
		IS_62SERIES=1
                AC_DEFINE([IS_72SERIES])
	else
		echo "$version_str" | egrep -o "7\.2\..+" > /dev/null
		if test "x$?" = "x0"; then
			AC_MSG_NOTICE([GCC 7.2.x compiler detected... ${MPC_CXX_COMPILER}])
			IS_72SERIES=1
                        AC_DEFINE([IS_72SERIES])
                else
                    echo "$version_str" | egrep -o "7\.3\..+" > /dev/null
		    if test "x$?" = "x0"; then
			AC_MSG_NOTICE([GCC 7.3.x compiler detected... ${MPC_CXX_COMPILER}])
			IS_72SERIES=1
                        AC_DEFINE([IS_73SERIES])
		    fi
                fi
	fi
fi

if test "$IS_62SERIES" = "0" -a "$IS_48SERIES" = "0" -a "$IS_72SERIES" = "0" -a "$IS_73SERIES" = "0"; then
	AC_MSG_ERROR([The CC version is not compatible with supported version (4.8.x, 6.2.x, 7.2.x or 7.3.x)])
fi

AM_CONDITIONAL([IS_48SERIES], [test "$IS_48SERIES" != "0"])
AM_CONDITIONAL([IS_62SERIES], [test "$IS_62SERIES" != "0"])
AM_CONDITIONAL([IS_72SERIES], [test "$IS_72SERIES" != "0"])
AM_CONDITIONAL([IS_73SERIES], [test "$IS_73SERIES" != "0"])


AC_OUTPUT
