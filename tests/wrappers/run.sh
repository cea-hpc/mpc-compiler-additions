#!/bin/sh

prefix="$1"

test -n "$prefix" || exit 42

# no priv
echo "
========= NOT PRIVATIZED"
($prefix/bin/apcc main.c && ./a.out) || exit 1
($prefix/bin/ap++ main.cpp && ./a.out) || exit 1
#($prefix/apfortran main.f && ./a.out) || exit 1

echo "
========== PRIVATIZATION ENABLED"
($prefix/bin/apcc main.c -fmpc-privatize && ./a.out) || exit 1
($prefix/bin/ap++ main.cpp -fmpc-privatize && ./a.out) || exit 1
#($prefix/apfortran main.f -fmpc-privatize && ./a.out || exit 1


echo "
========== HLS MODE ENABLED"
($prefix/bin/apcc hls.c -fhls -fhls-verbose -fmpc-privatize && ./a.out) || exit 1

