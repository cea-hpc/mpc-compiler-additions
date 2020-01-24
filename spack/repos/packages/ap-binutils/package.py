# Copyright 2013-2020 Lawrence Livermore National Security, LLC and other
# Spack Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

from spack import *
import glob


class ApBinutils(AutotoolsPackage, GNUMirrorPackage):
    """GNU binutils, which contain the linker, assembler, objdump and others
       This version mirrors the regular Binutils package, with the addition
       of Autopriv patches, for automatic deployment."""

    homepage = "http://www.gnu.org/software/binutils/"
    gnu_mirror_path = "binutils/binutils-2.27.tar.bz2"

    version('2.27', sha256='369737ce51587f92466041a97ab7d2358c6d9e1b6490b3940eb09fb0a9a6ac88')
    version('2.25', sha256='22defc65cfa3ef2a3395faaea75d6331c6e62ea5dfacfed3e2ec17b08c882923')

    variant('plugins', default=False,
            description="enable plugins, needed for gold linker")
    variant('gold', default=True, description="build the gold linker")
    variant('libiberty', default=False, description='Also install libiberty.')
    variant('nls', default=True, description='Enable Native Language Support')
    variant('headers', default=False, description='Install extra headers (e.g. ELF)')

    patch('cr16.patch', when='@:2.29.1')
    patch('update_symbol-2.26.patch', when='@2.26')


    ###### MPC-ONLY
    patch('binutils-2.25.patch', when="@2.25")
    patch('binutils-2.27.patch', when="@2.27")
    ###### END MPC-ONLY


    depends_on('zlib')
    depends_on('gettext', when='+nls')

    # Prior to 2.30, gold did not distribute the generated files and
    # thus needs bison, even for a one-time build.
    depends_on('m4', type='build', when='@:2.29.99 +gold')
    depends_on('bison', type='build', when='@:2.29.99 +gold')

    def configure_args(self):
        spec = self.spec

        configure_args = [
            '--disable-dependency-tracking',
            '--disable-werror',
            '--enable-multilib',
            '--enable-shared',
            '--enable-64-bit-bfd',
            '--enable-targets=all',
            '--with-system-zlib',
            '--with-sysroot=/',
        ]

        if '+gold' in spec:
            configure_args.append('--enable-gold')

        if '+plugins' in spec:
            configure_args.append('--enable-plugins')

        if '+libiberty' in spec:
            configure_args.append('--enable-install-libiberty')

        if '+nls' in spec:
            configure_args.append('--enable-nls')
            configure_args.append('LDFLAGS=-lintl')
        else:
            configure_args.append('--disable-nls')

        # To avoid namespace collisions with Darwin/BSD system tools,
        # prefix executables with "g", e.g., gar, gnm; see Homebrew
        # https://github.com/Homebrew/homebrew-core/blob/master/Formula/binutils.rb
        if spec.satisfies('platform=darwin'):
            configure_args.append('--program-prefix=g')

        return configure_args

    @run_after('install')
    def install_headers(self):
        # some packages (like TAU) need the ELF headers, so install them
        # as a subdirectory in include/extras
        if '+headers' in self.spec:
            extradir = join_path(self.prefix.include, 'extra')
            mkdirp(extradir)
            # grab the full binutils set of headers
            install_tree('include', extradir)
            # also grab the headers from the bfd directory
            for current_file in glob.glob(join_path(self.build_directory,
                                                    'bfd', '*.h')):
                install(current_file, extradir)

    def flag_handler(self, name, flags):
        # To ignore the errors of narrowing conversions for
        # the Fujitsu compiler
        if name == 'cxxflags'\
           and (self.compiler.name == 'fj' or self.compiler.name == 'clang')\
           and self.version <= ver('2.31.1'):
            flags.append('-Wno-narrowing')
        return (flags, None, None)
