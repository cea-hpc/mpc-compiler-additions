# Copyright 2013-2020 Lawrence Livermore National Security, LLC and other
# Spack Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

# ----------------------------------------------------------------------------
# If you submit this package back to Spack as a pull request,
# please first remove this boilerplate and all FIXME comments.
#
# This is a template package file for Spack.  We've put "FIXME"
# next to all the things you'll want to change. Once you've handled
# them, you can save this file and test your package like this:
#
#     spack install libextls
#
# You can edit this file again by typing:
#
#     spack edit libextls
#
# See the Spack documentation for more information on packaging.
# ----------------------------------------------------------------------------

from spack import *


def _gcc_patch_variants():
    return {@VARIANTS_LIST@}


def _gcc_version():
    return {@GCC_VERSIONS_LIST@}


class Autopriv(AutotoolsPackage):
    """Privatized compiler, relying over GCC to provide the privatization
    process, allowing process-based codes to run inside threads."""

    homepage = "http://mpc.hpcframework.com"
    url = "https://france.paratools.com/autopriv/autopriv-0.5.0.tar.gz"
    version('@AUTOPRIV_VERSION@',
            sha256='@SHA256SUM@')

    depends_on("hwloc@2.2.0")
    depends_on("openpa")
    depends_on("libelf", when="+libelf")

    variant("debug", default=False, description="Enable debug mode")
    variant("libelf", default=True, description="Use libelf for symbol introspection")
    variant("ccpatch", default=True,
            description="Install and deploy embedded GNU GCC privatizing compiler")
    variant("gcc_version", default="@DEFAULT_GCC@",
            description='GCC version to be deployed',
            values=_gcc_version())
    @SPACK_VARIANT_FLAGS@

    @GCC_VERSION_CONFLICTS@

    def configure_args(self):
        spec = self.spec
        options = [
            '--opts="--with-openpa={0} --with-hwloc={1}"'.format(
                spec['openpa'].prefix, spec['hwloc'].prefix)
        ]

        if spec.satisfies("+debug"):
            options.extend(['--enable-debug'])

        if spec.satisfies("-ccpatch"):
            options.extend(['--disable-gcc'])
        else:
            options.extend(['--gcc-version={}'.format(spec.variants['gcc_version'].value)])

        @SPACK_VARIANT_CONF_FLAGS@

        return options
