#!/bin/sh

VERSION="`./.autopriv_version`"
FILE="autopriv-${VERSION}.tar.gz"

echo -n "Generating archive for autopriv-${VERSION}... "
git archive --format=tar.gz --prefix=autopriv-${VERSION}/ origin/master > $FILE
echo "OK"

echo "./${FILE}"