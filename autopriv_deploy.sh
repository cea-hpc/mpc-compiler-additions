#!/bin/bash


tar_rootdir()
{
	tar tf "$1" | head -1 | sed -e "s/\/.*//"
}
exit_error()
{
	printf "Error: $@\n" >&2
	exit 42
}

safe_exec()
{
	echo "$ $@"
	if [ "$RUN" = "yes" ]; then
		eval "$@ $SILENT" || exit_error "$@ returned an non-zero exit code ! ABORT"
	fi
}

usage()
{
	printf "Usage: $0 [-i <install-prefix>] [-b <build-prefix>] [-jX] [-sShDf]\n"
	printf "     -h          Print this message.\n"
	printf "     -J<number>  Number of jobs running simultaneously (see \`man make\`)"
	printf "     -d          Dry-run, only print commands that would be run.\n"
	printf "     -s          Make output silent. (do not output build logs).\n"
	printf "     -S          Make completely silent (even errors).\n"
	printf "     -f          Erase any previous installation, if any.\n"
	printf "     -b <prefix> Select the directory where autopriv and deps will be built.\n"
	printf "                 Set to '$SRCDIR/build' by default.\n"
	printf "     -i <prefix> Select the directory where autopriv and deps will be installed.\n"
	printf "                 Set to '$SRCDIR/build/install' by default.\n"
}

RUN="yes"
FORCE="no"
SILENT=""
SRCDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
DSTDIR="$SRCDIR/build/install"
BLDIR="${SRCDIR}/build/"
MAKE_J=""
gcc_deps="gmp-* mpfr-* mpc-* isl-*"

while getopts "i:j:b:hdsSf" arg;
do
	case $arg in
		i)
			DSTDIR=${OPTARG}
			;;
		j)
			MAKE_J="-j${OPTARG}"
			;;
		b)
			BLDIR=${OPTARG}
			;;
		s)
			SILENT=" > /dev/null"
			;;
		S)
			SILENT=" > /dev/null 2>&1"
			;;
		d)
			RUN="no"
			;;
		f)
			FORCE="yes"
			;;
		h)
			usage
			exit 0
			;;
		*)
			usage
			exit_error "Unknown argument --> '$arg'"
			;;
	esac
done

echo "############################"
echo SRC - $SRCDIR
echo BLD - $BLDIR
echo DST - $DSTDIR
echo "############################"

test -n "$SRCDIR" || exit_error "SRCDIR should not be empty !"
test -n "$BLDIR" || exit_error "BLDIR should not be empty !"
test -n "$DSTDIR" || exit_error "DSTDIR should not be empty !"

[ -f "$DSTDIR/bin/autopriv.sh" -a "$FORCE" != "yes" ] && exit_error "Cannot erase a previous installation. Please use '-f' to force it."
[ -d "$BLDIR" ] && safe_exec rm -rf $BLDIR
safe_exec mkdir -p $BLDIR

safe_exec tar -xf $SRCDIR/deps/gcc-*.tar.* -C $BLDIR/
gcc_rootname=$(tar_rootdir $SRCDIR/deps/gcc-*.tar.*)
safe_exec tar -xf $SRCDIR/deps/binutils-*.tar.* -C $BLDIR/
binutils_rootname=$(tar_rootdir $SRCDIR/deps/binutils-*.tar.*)
for dep in ${gcc_deps}; do
	safe_exec tar -xf $SRCDIR/deps/$dep -C $BLDIR/
	rootname=$(tar_rootdir $SRCDIR/deps/$dep)
	safe_exec mv $BLDIR/$rootname $BLDIR/$gcc_rootname/${dep//-*/}
done

safe_exec mkdir -p $BLDIR/$binutils_rootname/build
safe_exec mkdir -p $BLDIR/$gcc_rootname/build
safe_exec mkdir -p $BLDIR/autopriv

safe_exec cd $BLDIR/$binutils_rootname
safe_exec patch -p1 < $SRCDIR/deps/patches/${binutils_rootname}.patch
safe_exec cd build
safe_exec ../configure --prefix=$DSTDIR
safe_exec make ${MAKE_J}
safe_exec make install ${MAKE_J}

safe_exec cd $BLDIR/$gcc_rootname
safe_exec patch -p1 < $SRCDIR/deps/patches/${gcc_rootname}.patch
safe_exec cd build
safe_exec ../configure --prefix=$DSTDIR --enable-languages=c,c++,fortran --disable-multilib --disable-bootstrap --program-suffix=-ap
safe_exec make ${MAKE_J}
safe_exec make install ${MAKE_J}

safe_exec cd $BLDIR/autopriv
safe_exec $SRCDIR/configure --prefix=$DSTDIR CC=$DSTDIR/bin/gcc-ap CXX=$DSTDIR/bin/g++-ap FC=$DSTDIR/bin/gfortran-ap
safe_exec make ${MAKE_J}
safe_exec make install ${MAKE_J}
