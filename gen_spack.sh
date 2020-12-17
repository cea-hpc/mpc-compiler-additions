#!/bin/sh

#
# Script environment
#

SCRIPTPATH=$(dirname "$(readlink -f "$0")")

# First load all helper functions
if test -f "${SCRIPTPATH}/config/helper.sh"; then
	# shellcheck source=config/helper.sh
	. "${SCRIPTPATH}/config/helper.sh"
else
	echo "FATAL could not load ${SCRIPTPATH}/config/helper.sh"
fi

# Config FILES
SOURCES_CONF="${SCRIPTPATH}/config/sources.txt"
assert_file_exists "$SOURCES_CONF"

VARIANTS_CONF="${SCRIPTPATH}/config/variants.txt"
assert_file_exists "$VARIANTS_CONF"

VERSIONS_CONF="${SCRIPTPATH}/config/versions.txt"
assert_file_exists "$VERSIONS_CONF"

PKG_TEMPLATE="${SCRIPTPATH}/config/spack_recipe.py"
assert_file_exists "$PKG_TEMPLATE"

PKG_OUTPUT="${SCRIPTPATH}/spack/repos/packages/autopriv/package.py"
assert_file_exists "$PKG_OUTPUT"

GCC_VERSIONS_LIST=""
for v in $(gcc_version_list)
do
    GCC_VERSIONS_LIST="\"$v\", ${GCC_VERSIONS_LIST}"
done

VARIANTS_LIST=""

for v in $(variant_list)
do
    VARIANTS_LIST="\"$v\", ${VARIANTS_LIST}"
done


SPACK_VARIANT_FLAGS=""

for v in $(variant_list)
do
    SPACK_VARIANT_FLAGS="${SPACK_VARIANT_FLAGS}\n    variant(\"$v\", default=True, description=\"Enable $v support\")"
done

SPACK_VARIANT_CONF_FLAGS=""

for v in $(variant_list)
do
    SPACK_VARIANT_CONF_FLAGS="${SPACK_VARIANT_CONF_FLAGS}\n        if spec.satisfies(\"-$v\"):\n            options.extend(['--disable-$v'])"
done


GCC_VERSION_CONFLICTS=""

for gccv in $(gcc_version_list)
do

    for variant in $(variant_list)
    do

        if variant_supports_gcc_version "$variant" "$gccv"; then
            true
        else
            GCC_VERSION_CONFLICTS="${GCC_VERSION_CONFLICTS}\n    conflicts(\"+${variant}\",\n              when=\"gcc_version=$gccv\",\n              msg=\"gcc_version=$gccv is not compatible with variant ${variant}\")"
        fi
    done
done

AUTOPRIV_VERSION=$(${SCRIPTPATH}/.autopriv_version)

TARBALL="${SCRIPTPATH}/autopriv-${AUTOPRIV_VERSION}.tar.gz"

if test ! -f "${TARBALL}"; then
    ${SCRIPTPATH}/gen_archive.sh
fi

assert_file_exists "${TARBALL}"

SHA256SUM=$(sha256sum "${TARBALL}" |cut -d " " -f 1)

echo $SHA256SUM

DEFAULT_GCC_VERSION="$(get_default_gcc_version)"

sed  "s/@VARIANTS_LIST@/$VARIANTS_LIST/g" < "${SCRIPTPATH}/config/spack_recipe.py" | sed "s/@GCC_VERSIONS_LIST@/$GCC_VERSIONS_LIST/g" | sed "s/@SPACK_VARIANT_FLAGS@/$SPACK_VARIANT_FLAGS/g" | sed "s/@SPACK_VARIANT_CONF_FLAGS@/$SPACK_VARIANT_CONF_FLAGS/g" | sed "s/@GCC_VERSION_CONFLICTS@/$GCC_VERSION_CONFLICTS/g" | sed "s/@AUTOPRIV_VERSION@/$AUTOPRIV_VERSION/g" | sed "s/@SHA256SUM@/$SHA256SUM/g" | sed "s/@DEFAULT_GCC@/$DEFAULT_GCC_VERSION/g" | tee ${PKG_OUTPUT}
