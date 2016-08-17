
ARB will compile with several versions of gcc.

Compilation is frequently tested with

            gcc version   used in ubuntu

            gcc 4.2.4      8.04 LTS hardy
            gcc 4.3.3      9.04     jaunty
            gcc 4.4.3     10.04     lucid

The main Makefile in the ARB directory lists all supported versions
(currently starting with gcc 3.2) and complains if you try to use
an untested gcc version.

You may try to simply add the untested version number to the Makefile
and try compilation. Especially if the version lays somewhere inbetween
the listed versions your chances are good that it will work.

If you encounter compilation problems with any of the versions listed
in the Makefile, please report to devel@arb-home.de


If your gcc version refuses to compile ARB, you need to install an
additional gcc. 


Here a short howto (using gcc 4.5.2 as example):

- download (e.g.)

      mkdir new-dir
      cd new-dir
      wget 'http://gcc.cybermirror.org/releases/gcc-4.5.2/gcc-core-4.5.2.tar.bz2'
      wget 'http://gcc.cybermirror.org/releases/gcc-4.5.2/gcc-g++-4.5.2.tar.bz2'

- unpack into directory 'src':

      mkdir src
      cd src
      tar -jxvf ../gcc-core-4.5.2.tar.bz2
      tar -jxvf ../gcc-g++-4.5.2.tar.bz2

- create directory 'objs'

      cd ..
      mkdir objs

- use bash:

      bash

- configure gcc:

      cd objs
      ../src/gcc-4.5.2/configure --prefix=/opt/gcc-4.5.2 --disable-nls

- build gcc:

      make bootstrap

- install gcc:

      su
      make install

- prefix
      /opt/gcc-4.5.2/bin
  to your PATH environment variable.

- compile ARB


-------------------------------------------
problems that may occur while compiling gcc
-------------------------------------------

 - configure is complaining about wrong libmfc

   - download, compile and install recent version
     (further assuming it was installed into /usr/local)
   - before configuring gcc set

     LD_OPTIONS='-L/usr/local/lib -R/usr/local/lib'
     export LD_OPTIONS
     LDFLAGS='-L/usr/local/lib -R/usr/local/lib'
     export LDFLAGS

   - run configure with
         --with-mpfr=/usr/local
     option
   - continue with 'make bootstrap' like above
