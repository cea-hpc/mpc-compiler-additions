#!/bin/sh

COMPILATION_PREFIX=@COMPILATION_PREFIX@
RUNTIME_PREFIX=@RUNTIME_PREFIX@

#vars should be cleaned before set 
##################################

if test ! -z "${COMPILATION_PREFIX}" ; then
	cprefix=`echo "$COMPILATION_PREFIX" | sed -e 's/\//\\\\\//g'`

	PATH=`echo ${PATH} | awk -v RS=: -v ORS=: '/'"$cprefix"'/ {next} {print}' | sed 's/:*$//'`
	LD_LIBRARY_PATH=`echo ${LD_LIBRARY_PATH} | awk -v RS=: -v ORS=: '/'"$cprefix"'/ {next} {print}' | sed 's/:*$//'`
	LIBRARY_PATH=`echo ${LIBRARY_PATH} | awk -v RS=: -v ORS=: '/'"$cprefix"'/ {next} {print}' | sed 's/:*$//'`
	MANPATH=`echo ${MANPATH} | awk -v RS=: -v ORS=: '/'"$cprefix"'/ {next} {print}' | sed 's/:*$//'`
fi

if test ! -z "${RUNTIME_PREFIX}"; then
	rprefix=`echo "$RUNTIME_PREFIX" | sed -e 's/\//\\\\\//g'`
	PATH=`echo ${PATH} | awk -v RS=: -v ORS=: '/'"$rprefix"'/ {next} {print}' | sed 's/:*$//'`
	LD_LIBRARY_PATH=`echo ${LD_LIBRARY_PATH} | awk -v RS=: -v ORS=: '/'"$rprefix"'/ {next} {print}' | sed 's/:*$//'`
	LIBRARY_PATH=`echo ${LIBRARY_PATH} | awk -v RS=: -v ORS=: '/'"$rprefix"'/ {next} {print}' | sed 's/:*$//'`
	MANPATH=`echo ${MANPATH} | awk -v RS=: -v ORS=: '/'"$rprefix"'/ {next} {print}' | sed 's/:*$//'`
fi



export PATH=$COMPILATION_PREFIX/bin:$RUNTIME_PREFIX/bin:$PATH
export INCLUDE_PATH=$COMPILATION_PREFIX/include:$RUNTIME_PREFIX/include:$INCLUDE_PATH
export LD_LIBRARY_PATH=$COMPILATION_PREFIX/lib:$COMPILATION_PREFIX/lib64::$RUNTIME_PREFIX/lib:$RUNTIME_PREFIX/lib64:$LD_LIBRARY_PATH
export MANPATH=$COMPILATION_PREFIX/man:$RUNTIME_PREFIX/man:$MANPATH

export EXTLS_CFLAGS="-I$RUNTIME_PREFIX/include"
export EXTLS_LDFLAGS="-L$RUNTIME_PREFIX/lib -L/$RUNTIME_PREFIX/lib64 -Wl,-rpath=$RUNTIME_PREFIX/lib"
