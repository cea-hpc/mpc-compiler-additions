#!/bin/sh

check_program()
{
	which $1 > /dev/null 2>&1
	if test "$?" != "0";
	then
		printf "Error: $1 not found ! Exit...\n"
		exit
	fi
}

get_version_number()
{
	. $PWD/.extls_version
}

if test ! -f $PWD/.extls_version; then
	printf "This script has to be run from libextls sources root tree !\n"
	printf "Please rerun from root: ./tools/gen_archive.sh\n"
	exit 1
fi

git diff --quiet HEAD
if test "$?" != "0";
then
	printf "You have some changes in your working tree (staged or not).\n"
	printf "I'm not sure what you want to do. Please commit or stash your\n"
	printf "modifications before trying to generate a tarball.\n"
	exit 1
fi

check_program tar
check_program gzip

version="`get_version_number`"
git archive --format=tar --prefix=libextls-${version}/ HEAD | gzip > libextls-${version}.tar.gz
exit $?
