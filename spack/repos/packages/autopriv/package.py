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
    return {"workshare", "autopriv", }


def _gcc_version():
    return {"4.8.5", "6.2.0", "7.4.0", "7.3.0", "7.5.0", "8.1.0", "8.2.0", "8.3.0", "8.4.0", "9.1.0", "9.2.0", "9.3.0", "10.1.0", "10.2.0", }


class Autopriv(AutotoolsPackage):
    """Privatized compiler, relying over GCC to provide the privatization
    process, allowing process-based codes to run inside threads."""

    homepage = "http://mpc.hpcframework.com"
    url = "https://france.paratools.com/autopriv/autopriv-0.5.0.tar.gz"
    version('0.6.2',
            sha256='6054c9531f0545f0ce843fad03a1c6852305bf95b84e2971811b6c3ae5621a77')

    depends_on("hwloc@2.2.0")
    depends_on("openpa")
    depends_on("libelf", when="+libelf")

    variant("debug", default=False, description="Enable debug mode")
    variant("libelf", default=True, description="Use libelf for symbol introspection")
    variant("ccpatch", default=True,
            description="Install and deploy embedded GNU GCC privatizing compiler")
    variant("gcc_version", default="7.3.0",
            description='GCC version to be deployed',
            values=_gcc_version())
    
    variant("autopriv", default=True, description="Enable autopriv support")
    variant("workshare", default=True, description="Enable workshare support")

    
    conflicts("+workshare",
              when="gcc_version=10.2.0",
              msg="gcc_version=10.2.0 is not compatible with variant workshare")
    conflicts("+workshare",
              when="gcc_version=10.1.0",
              msg="gcc_version=10.1.0 is not compatible with variant workshare")
    conflicts("+workshare",
              when="gcc_version=9.3.0",
              msg="gcc_version=9.3.0 is not compatible with variant workshare")
    conflicts("+workshare",
              when="gcc_version=9.2.0",
              msg="gcc_version=9.2.0 is not compatible with variant workshare")
    conflicts("+workshare",
              when="gcc_version=9.1.0",
              msg="gcc_version=9.1.0 is not compatible with variant workshare")
    conflicts("+workshare",
              when="gcc_version=8.4.0",
              msg="gcc_version=8.4.0 is not compatible with variant workshare")
    conflicts("+workshare",
              when="gcc_version=8.3.0",
              msg="gcc_version=8.3.0 is not compatible with variant workshare")
    conflicts("+workshare",
              when="gcc_version=8.2.0",
              msg="gcc_version=8.2.0 is not compatible with variant workshare")
    conflicts("+workshare",
              when="gcc_version=8.1.0",
              msg="gcc_version=8.1.0 is not compatible with variant workshare")
    conflicts("+workshare",
              when="gcc_version=7.5.0",
              msg="gcc_version=7.5.0 is not compatible with variant workshare")
    conflicts("+workshare",
              when="gcc_version=7.4.0",
              msg="gcc_version=7.4.0 is not compatible with variant workshare")
    conflicts("+workshare",
              when="gcc_version=6.2.0",
              msg="gcc_version=6.2.0 is not compatible with variant workshare")
    conflicts("+workshare",
              when="gcc_version=4.8.5",
              msg="gcc_version=4.8.5 is not compatible with variant workshare")

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

        
        if spec.satisfies("-autopriv"):
            options.extend(['--disable-autopriv'])
        if spec.satisfies("-workshare"):
            options.extend(['--disable-workshare'])

        return options
