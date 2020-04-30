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


class Autopriv(AutotoolsPackage):
    """Privatized compiler, relying over GCC to provide the privatization
    process, allowing process-based codes to run inside threads."""

    homepage = "http://mpc.hpcframework.com"
    url = "https://france.paratools.com/autopriv/autopriv-0.5.0.tar.gz"
    version('0.5.0', sha256='d04f2b76ab7d74e3a08a2c39a3015c16ef8cadb318fd9ab182ebdf1d473e5200')

    depends_on("gmp", type="build") # need to keep it explicit for dynpriv build
    depends_on("ap-gcc@:7.3.0", when="+ccpatch")
    depends_on("hwloc@1.11.11")
    depends_on("openpa")
    depends_on("libelf", when="+libelf")

    variant("debug", default=False, description="Enable debug mode")
    variant("libelf", default=True, description="Use libelf for symbol introspection")
    variant("ccpatch", default=True, description="Install and deploy embedded GNU GCC privatizing compiler")

    def configure_args(self):
        spec = self.spec
        options = [
            '--with-openpa={0}'.format(spec['openpa'].prefix),
            '--with-hwloc={0}'.format(spec['hwloc'].prefix),
        ]

        if spec.satisfies("+debug"):
            options.extend(['--enable-debug'])

        if spec.satisfies("-ccpatch"):
            options.extend(['--disable-dynpriv'])

        return options

    def setup_build_environment(self, env):
        
        if self.spec.satisfies("+ccpatch"):
            env.set('CC', self.spec['ap-gcc'].apcc)
            env.set('CXX', self.spec['ap-gcc'].apcxx)
            env.set('FC', self.spec['ap-gcc'].apfc)

            updated_cppflags="-I"+self.spec['gmp'].prefix+'/include'
            updated_cflags= \
                    " -I"+self.spec['hwloc'].prefix+"/include"+ \
                    " -I"+self.spec['openpa'].prefix+"/include"

            updated_ldflags= \
                    " -L"+self.spec['hwloc'].prefix+"/lib -Wl,-rpath="+self.spec['hwloc'].prefix+"/lib -lhwloc"+ \
                    " -L"+self.spec['openpa'].prefix+"/lib -Wl,-rpath="+self.spec['openpa'].prefix+"/lib -lopenpa"

            if self.spec.satisfies("+libelf"):
                updated_cflags=" -I"+self.spec['libelf'].prefix+"/include"
                updated_ldflags=" -L"+self.spec['libelf'].prefix+'/lib -Wl,-rpath='+self.spec['libelf'].prefix+'/lib -lelf'

            env.set('CPPFLAGS', updated_cppflags)
            env.set('CFLAGS', updated_cflags)
            env.set('LDFLAGS', updated_ldflags)
