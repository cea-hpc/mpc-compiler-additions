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
    version('0.5.0', sha256='7c581f33d9981ea118b942d3d6e8043aa55ecce18a0c5c1b54bc6ebb565e12e4')



    depends_on("gmp", type="build") # need to keep it explicit for dynpriv build
    depends_on("ap-gcc")
    depends_on("hwloc@1.11.11")
    depends_on("openpa")
    depends_on("libelf", when="+libelf")

    variant("debug", default=False, description="Enable debug mode")
    variant("libelf", default=True, description="Use libelf for symbol introspection")

    def configure_args(self):
        spec = self.spec
        options = [
            '--with-openpa={0}'.format(spec['openpa'].prefix),
            '--with-hwloc={0}'.format(spec['hwloc'].prefix),
        ]

        if spec.satisfies("+debug"):
            options.extend(['--enable-debug'])

        return options

    def setup_build_environment(self, env):
        env.set('CC', self.spec['ap-gcc'].apcc)
        env.set('CXX', self.spec['ap-gcc'].apcxx)
        env.set('FC', self.spec['ap-gcc'].apfc)
