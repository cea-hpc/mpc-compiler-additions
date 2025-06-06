#!/bin/sh


set -e

SCRIPTPATH=$(dirname "$(readlink -f "$0")")

if test ! -f "$(basename "$0")"; then
    echo "=================================================================="
    echo "Please run this script from mpc-compiler-additions' root directory"
    echo "=================================================================="
    exit 1
fi

set -x

echo "Cleaning timestamps for dist"
find . -type f -print0 | xargs -0 touch -r ./configure

VERSION="$(./.mpc_comp_additions_version)"
FILE="${PWD}/mpc-compiler-additions-${VERSION}.tar.gz"

echo -n "Generating archive for autopriv-${VERSION}... "
git archive --format=tar.gz --prefix="mpc-compiler-additions-${VERSION}/" HEAD > "$FILE"
echo "OK"




TMPDIR=$(mktemp -d)
cd "${TMPDIR}" || exit 42

tar xf "${FILE}"
cd ./mpc-compiler-additions-* || exit 42

echo "Downloading dependencies.."

"${SCRIPTPATH}/configure" --download --no-home-autopriv "$@"

echo "Inserting dependencies ..."

cd .. || exit 42

tar czf "${FILE}" ./mpc-compiler-additions-*

rm -fr "${TMPDIR}"

echo "./${FILE}"
