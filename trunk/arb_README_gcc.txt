
ARB will NOT compile with gcc 3.2 yet (and so it won't with SuSE 8.1).

To compile you'll need to install a different version of gcc.
Compilation was tested with gcc 2.95.3 and gcc versions higher or equal to gcc 3.3.3


If you have a unsupported gcc as system compiler, follow the instructions below to install
gcc version 2.95.3 (we install it in /opt/gcc-2.95):

- download (i.e.)

	- ftp://ftp.leo.org/pub/comp/os/unix/gnu/gcc/gcc-2.95.3/gcc-core-2.95.3.tar.gz  (8.5 Mb)
	- ftp://ftp.leo.org/pub/comp/os/unix/gnu/gcc/gcc-2.95.3/gcc-g++-2.95.3.tar.gz	(1.5 Mb)

	or

	- http://gd.tuwien.ac.at/gnu/sourceware/gcc/releases/gcc-2.95.3/gcc-core-2.95.3.tar.gz
	- http://gd.tuwien.ac.at/gnu/sourceware/gcc/releases/gcc-2.95.3/gcc-g++-2.95.3.tar.gz

- unpack into directory 'your-gcc-source'
- create directory 'your-gcc-objects'

- configure gcc:

	cd your-gcc-objects
	../your-gcc-source/configure --prefix=/opt/gcc-2.95

        [If you'd like to see english error messages use '--disable-nls' ]

- build gcc:

	make bootstrap

	su
	make install

- prefix
	/opt/gcc-2.95/bin
  to your PATH environment variable.

- compile ARB

