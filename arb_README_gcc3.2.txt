
ARB will NOT compile with gcc 3.2 yet (and so it won't with SuSE 8.1). 
To compile you'll need to install a parallel gcc 2.95.3 (i.e. in /opt/gcc-2.95 )

To do this

- download (i.e.)

	- ftp://ftp.leo.org/pub/comp/os/unix/gnu/gcc/gcc-2.95.3/gcc-core-2.95.3.tar.gz  (8.5 Mb)
	- ftp://ftp.leo.org/pub/comp/os/unix/gnu/gcc/gcc-2.95.3/gcc-g++-2.95.3.tar.gz	(1.5 Mb)

	or 

	- http://gd.tuwien.ac.at/gnu/sourceware/gcc/releases/gcc-2.95.3/gcc-core-2.95.3.tar.gz
	- http://gd.tuwien.ac.at/gnu/sourceware/gcc/releases/gcc-2.95.3/gcc-g++-2.95.3.tar.gz

- follow the installation instructions 

  You want to build gcc for destination /opt/gcc-2.95
  by calling configure like this

	cd your-gcc-objects
	../your-gcc-source/configure --prefix=/opt/gcc-2.95

	make bootstrap

	su
	make install

- prefix 
	/opt/gcc-2.95/bin 
  to your PATH environment variable.

- compile ARB

