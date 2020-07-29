#!/bin/sh

set -x

prefix="$1"
file="$2"

error()
{
	printf "Error: $@\n" 1>&2
	exit 42
}

echo "======> $@"
test -n "$prefix" || exit 42
test -n "$file" || exit 43

prog=apcc
case $file in
	*.c)
		prog=apcc
		;;
	*.C|*.cpp)
		prog=ap++
		;;
	*)
		error "Source file $file not recognized. Failed !"
		;;
esac
$prefix/bin/$prog $file -o nopriv || error "Failed during 'nopriv' build for $file"
$prefix/bin/$prog $file -o priv -fmpc-privatize -fhls -fhls-verbose || error "Failed during 'priv' build for $file"


for bin in nopriv priv
do
	./priv
	ret=$?
	test -f ./a.out && rm -f ./a.out
	test "$ret" = "0" || error "Failed to run ./a.out from $file"
done

exit 0
