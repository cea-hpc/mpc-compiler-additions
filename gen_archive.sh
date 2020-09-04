#!/bin/sh


set -e

SCRIPTPATH=$(dirname "$(readlink -f "$0")")

if test ! -f "$(basename "$0")"; then
    echo "==================================================="
    echo "Please run this script from autopriv root directory"
    echo "==================================================="
    exit 1
fi

set -x

VERSION="`./.autopriv_version`"
FILE="${PWD}/autopriv-${VERSION}.tar.gz"

echo -n "Generating archive for autopriv-${VERSION}... "
git archive --format=tar.gz --prefix=autopriv-${VERSION}/ HEAD > $FILE
echo "OK"




TMPDIR=$(mktemp -d)
cd "${TMPDIR}" || exit 42

tar xf "${FILE}"
cd ./autopriv-* || exit 42

echo "Downloading dependencies.."

${SCRIPTPATH}/configure --download "$@"

echo "Inserting dependencies ..."

cd .. || exit 42

tar czf "${FILE}" ./autopriv-*

rm -fr ${TMPDIR}

echo "./${FILE}"
