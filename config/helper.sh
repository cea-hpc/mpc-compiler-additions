#!/bin/sh

#
# General Helpers
#

exit_error()
{
	printf "Error: %s\n" "$@" >&2
	exit 42
}

trim()
{
	echo "$@" | xargs
}

SILENT="yes"
FAKE_RUN="no"
safe_exec()
{
	if test "x$SILENT" = "xno"; then
		REDIR="2>&1 | tee -a ./configure.log"
	else
		REDIR=">> ./configure.log 2>&1"
	fi

	echo "$ $*" | tee ./configure.log

	if [ "$FAKE_RUN" = "no" ]; then
		eval "$@" $REDIR || exit_error "$* Failed"
	fi
}

assert_file_exists()
{
	if test ! -f "$1"; then
		exit_error "$1 file not found."
	fi
}
# Check if command is available
#
# Args:
# - 1: cmd
have_cmd()
{
	command -v "$1" > /dev/null
}

# Check if an entry is in a list
#
# Args:
# - 1: needle
# - 2: haystack
in_list()
{
	echo "$2" | grep "$1" > /dev/null
}


#
# Config Files Common
#

# Cat a file filtering lines starting with #
# Args:
#   - Path to file
#
_config_file_clean()
{
    grep -v "^\#" < "$1" | sed -E "s/\s+/ /g"
}

#
# Package List Handling
#

# Get package list from source.txt
package_list()
{
    _config_file_clean "${SOURCES_CONF}" | cut -d " " -f 1
}

# Assert package is known
# Args:
# - 1: package name
assert_package()
{
	if in_list "$1" "$(package_list)"; then
		true
	else
		exit_error "No such package $1."
	fi
}

# Get package URL
# Args:
# - 1: package name
# - 2: version
#
package_download_address()
{
	_config_file_clean "${SOURCES_CONF}" | grep "$1" | cut -d " " -f 2 | sed "s/VERSION/${2}/g"
}

# Package Filename
# - 1: package name
# - 2: version
#
package_filename()
{
	package_download_address "$1" "$2" | rev | cut -d "/" -f 1 | rev
}

#
# Variants Handling
#

# Get variant list
variant_list()
{
    _config_file_clean "${VARIANTS_CONF}" | cut -d " " -f 1
}

# Compute enabled variant list
# Arg:
#  - 1 : List of disabled variants
#
enabled_variants()
{
	LIST=""
	for v in $(variant_list)
	do
		if in_list "$v" "$1"; then
			true
		else
			LIST="${v} ${LIST}"
		fi
	done

	trim "${LIST}"
}

# Assert that a variant is known
# Arg:
#  - 1 : variant
#
variant_assert()
{
    if in_list "$1" "$(variant_list)"; then
		true
	else
		exit_error "No such variant $1"
	fi
}

# Get all known GCC versions
gcc_version_list()
{
   _config_file_clean "${VERSIONS_CONF}" | cut -d " " -f 1
}


# Get variant GCC supported versions
# Arg:
#  - 1 : variant
#
variant_versions()
{
    _config_file_clean "${VARIANTS_CONF}" | grep "^$1" | cut -d " " -f 2-
}

# Return yes if the variant is supported by GCC
# Args:
# - 1: variant
# - 2: gcc_version
# Returns:
# - yes / no
#
variant_supports_gcc_version()
{
	variant_assert "$1"

	if in_list "$2" "$(variant_versions "$1")"; then
		return 0
	else
		return 1
	fi
}

#
# Version handling
#

# Check if a gcc version is known
# Args:
#  - $1 : version to check
#
#  Fail if not known
assert_gcc_version_known()
{
	VERSIONS=$(_config_file_clean "${VERSIONS_CONF}" | cut -d " " -f 1)
	if in_list "$1" "${VERSIONS}"; then
		true
	else
		exit_error "GCC version $1 not handled in ./config/versions.txt"
	fi
}

get_default_gcc_version()
{
    DEFAULT_VERSIONS=$(_config_file_clean "${VERSIONS_CONF}" | grep "yes" | cut -d " " -f 1)
    if test "x1" != "x$(echo "${DEFAULT_VERSIONS}" | wc -l)"; then
        VERSIONS="$(echo "$DEFAULT_VERSIONS" | xargs echo)"
        exit_error "More than one GCC version is set as default in config/versions.txt (${VERSIONS})."
    fi
    echo "${DEFAULT_VERSIONS}"
}

#
# Get package URL
# Args:
# - 1: package name
# - 2: gcc version
# Returns:
# - Version for package
package_version_for_gcc()
{
	assert_gcc_version_known "$2"
	INDEX=0

	case "$1" in
		gcc)
			INDEX=1
		;;
		binutils)
			INDEX=2
		;;
		gmp)
			INDEX=3
		;;
		mpc)
			INDEX=4
		;;
		mpfr)
			INDEX=5
		;;
		isl)
			INDEX=6
		;;
		hwloc)
			INDEX=7
		;;
		openpa)
			INDEX=8
		;;
		libelf)
			INDEX=9
		;;
		*)
			exit_error "No such package $1"
	esac

	_config_file_clean "${VERSIONS_CONF}" | grep "^$2" | cut -d " " -f "${INDEX}"

}

#
# Patching
#

# Get patch
# Args:
# - 1 : variant
# - 2 : dependency
# - 3 : version
get_patch()
{
	SCRIPTPATH=$(dirname "$(readlink -f "$0")")
	PATCH_ROOT="${SCRIPTPATH}/config/patches/"

	if test ! -d "${PATCH_ROOT}"; then
		exit_error "Could not find patch directory: ${PATCH_ROOT}"
	fi

	CANDIDATE="${PATCH_ROOT}/${1}/${2}/${3}.patch"

	if test -f "${CANDIDATE}"; then
		echo "${CANDIDATE}"
	fi
}



# Patch a dependency for a given variant & version
# Args:
# - 1: dep extraction path
# - 2: dep name
# - 3: dep version
# - 4: variant to patch for
patch_dependency()
{
	if test ! -d "$1"; then
		exit_error "Could not find $1 for pathing ($2@$3) for variant $4"
	fi

	# Now check
	PATCH=$(get_patch "$4" "$2" "$3")

	if test -n "${PATCH}"; then
		echo "# Applying $2-$(basename "${PATCH}") for $4 ..."
		cd "$1" || exit_error "Failed to chdir to ${1}"
		safe_exec patch -p1 < "${PATCH}"
		cd -  || exit_error "Failed to chdir" > /dev/null
	fi

}


#
# Build
#

CPU_COUNT=4

# Set -j for make
# Args:
# - 1 : Nproc
make_set_cpu_count()
{
	CPU_COUNT=$1
}


TO_CLEAN_DIRS="./BUILD_TARGET_*"

makefile_add_clean_dir()
{
	TO_CLEAN_DIRS="${1} ${TO_CLEAN_DIRS}"
}

# Run configure for a subpackage and create build dir
# Args:
# - 1 : package name
# - 2 : package source dir
# - 3 : makefile install dependencies
# - 4+ : CONFIGURE ARGS
#
makefile_configure_package()
{

	NAME=$1
	SRCDIR=$2
	DEPS=$3

	if test ! -d "$SRCDIR"; then
		exit_error "Could not locate sources in ${SRCDIR}"
	fi


	shift
	shift
	shift

	BDIR="${PWD}/BUILD_${NAME}"
	safe_exec mkdir "${BDIR}"

	makefile_add_clean_dir "${BDIR}"

	PREFIXED_DEPS=""
	for d in ${DEPS}
	do
		PREFIXED_DEPS="BUILD_TARGET_install_${d} $PREFIXED_DEPS"
	done

	if test "x${SILENT}" = "xyes"; then
		REDIR=">> ${NAME}.log"
		FALLBACK="|| (cat ${NAME}.log; false)"
	else
		REDIR=""
		FALLBACK=""
	fi


	cat >> "${PWD}/Makefile" << EOF
BUILD_TARGET_configure_${NAME}: ${PREFIXED_DEPS}
	cd ${BDIR} && ${SRCDIR}/configure $@
	touch BUILD_TARGET_configure_${NAME}

BUILD_TARGET_build_${NAME}: BUILD_TARGET_configure_${NAME}
	+(\$(MAKE) -C ${BDIR} ${REDIR} ) ${FALLBACK}
	touch BUILD_TARGET_build_${NAME}

BUILD_TARGET_install_${NAME}: BUILD_TARGET_build_${NAME}
	+(\$(MAKE) -C ${BDIR} install ${REDIR} ) ${FALLBACK}
	touch BUILD_TARGET_install_${NAME}

EOF
}

MAKE_MAIN_PKG=""

makefile_set_as_main_package()
{
	MAKE_MAIN_PKG="BUILD_TARGET_install_${1} ${MAKE_MAIN_PKG}"
}

# Generate the check target
# Args:
# -1 : list of jobs with check target
#
makefile_gen_check()
{
	DEPS=""
	for j in $1
	do
		DEPS="BUILD_TARGET_install_${j} ${DEPS}"
	done
	cat >> "${PWD}/Makefile" << EOF
check: ${DEPS}
EOF
	for j in $1
	do
		BDIR="${PWD}/BUILD_${j}"
		echo "	+make -C ${BDIR} check" >> "${PWD}/Makefile"
	done
}


makefile_commit()
{
	if test ! -f "./Makefile"; then
		exit_error "Could not locate Makefile in makefile_commit"
	fi

	mv ./Makefile tmp_mk

	cat >> "${PWD}/Makefile" << EOF
all: ${MAKE_MAIN_PKG}

install: all

$(cat ./tmp_mk)

clean:
EOF

	for p in $TO_CLEAN_DIRS
	do
		echo "	rm -fr ${p}" >> "${PWD}/Makefile"
	done

	rm -fr ./tmp_mk

}

#
# Downloader
#

# Get package extraction rootdir
# Args:
# - 1: path to tarball
tar_rootdir()
{
	tar tf "$1" | head -1 | sed -e "s/\/.*//"
}

# Get package extraction rootdir
# Args:
# - 1: path to tarball
# - 2: expected extraction directory
extract_and_move()
{
	if test -d "./$2"; then
		exit_error "Directory ${PWD}/$2 already exists (consider running 'make clean')."
	fi

	ROOT=$(tar_rootdir "$1")

	safe_exec tar xf "$1"
	safe_exec mv "${ROOT}" "$2"
	makefile_add_clean_dir "$(readlink -f "$2")"
}

# Extract and patch sources
# Args:
# - 1 : path to tarball
# - 2 : extraction dir
# - 3 : dependency name
# - 4 : GCC version
prepare_sources()
{
	PVERSION=$(package_version_for_gcc "$3" "$4")
	extract_and_move "$1" "$2"

	for v in $(variant_list)
	do
		patch_dependency "$2" "$3" "$PVERSION" "$v"
	done
}

# Get package from URL, current dir or PROJECT
# Args:
# - 1: package name
# - 2: gcc version
# Return:
# - TARBALL_PATH path to downloaded tarball
TARBALL_PATH=""
download_dep()
{
	assert_package "$1"
	PVERSION=$(package_version_for_gcc "$1" "$2")
	FNAME=$(package_filename "$1" "${PVERSION}")
	if test -f "${PWD}/${FNAME}"; then
		echo "# ${FNAME} found, skipping download."
		TARBALL_PATH="${PWD}/${FNAME}"
	elif test -f "${SCRIPTPATH}/${FNAME}"; then
		echo "# ${FNAME} found in project, skipping download."
		TARBALL_PATH="${SCRIPTPATH}/${FNAME}"
	elif test -f "${HOME}/.autopriv/${FNAME}"; then
		echo "# ${FNAME} found in user's HOME, skipping download."
		TARBALL_PATH="${HOME}/.autopriv/${FNAME}"
	else
		echo "# Downloading $1 ..."
		if have_cmd "wget"; then
			safe_exec wget "$(package_download_address "$1" "${PVERSION}")"
		elif have_cmd "curl"; then
			safe_exec curl -L "$(package_download_address "$1" "${PVERSION}")" -o "./${FNAME}"
		else
			exit_error "Curl or wget is required for dep download or use --download."
		fi
		#(TARBALL_PATH is the return value)
		# shellcheck disable=SC2034
		TARBALL_PATH="${PWD}/${FNAME}"
	fi
}

# Get all packages from URL, current dir or PROJECT
# Args:
# - 1: gcc version
# Return:
# - TARBALL_PATH path to downloaded tarball
download_all_deps()
{
	for p in $(package_list)
	do
		download_dep "$p" "$1"
		# If files were elsewhere copy them in local dir
		if test ! -f "$(basename "$TARBALL_PATH")"; then
			cp "${TARBALL_PATH}" .
		fi
	done
}

