#!/bin/sh

LIST_TO_HASH="`git ls-files -c`"

construct_version()
{
	. $PWD/.extls_version
	#echo "${MAJOR}.${MINOR}.${PATCH}"
}

if test ! -f $PWD/.extls_version; then
	echo "This script has to be run from libextls sources root tree !"
	echo "Please rerun from root: ./tools/gen_hashes.sh"
	exit 1
fi

echo "# This file contains MD5 checksums of the files for the version `construct_version`" > MD5SUMS
echo "# We suggest you to use: 'md5sum --quiet -c MD5SUMS'" >> MD5SUMS

for elt in $LIST_TO_HASH;
do
	if test "$elt" = "MD5SUMS";
	then
		continue;
	fi
	md5sum $elt >> MD5SUMS
done
