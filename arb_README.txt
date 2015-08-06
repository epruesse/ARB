
Welcome to the ARB 6 Sequence Analysis Suite 


System Requirements
------------------- 

  Minimal (demo.arb):
      Operating System: Linux or OSX
      CPU:              any
      Memory:           1 GB
      Hard drive:       200 MB

  Recommended (SILVA SSU NR99 r115):
      Operating System: Linux (Ubuntu 12.04 or newer) 
                        OSX (10.5.8 or newer)
      CPU:              >2 GHz multi-core
      Memory:           16 GB (or more!)
      Hard drive:       10 GB

  Notes:
  The resources required for ARB to run smoothly depend on the size of the
  database(s) you intend to work with. Large databases, especially the SSU
  databases provided by the SILVA project, require large amounts of fast
  memory.


Installation using Package Managers:
----------------------------------

  OSX:
    The recommended approach is to install MacPorts (macports.org) and use
    the ports command to install arb: "sudo port install arb"

  Debian Linux:
    ARB is available in Debian Testing in section "non-free". Install ARB
    using "apt-get install arb" or your preferred GUI front-end to apt-get. 


Installation from Pre-Compiled Binaries:
----------------------------------------

  1) Download "arb_install.sh" and the arb-*.tgz matching your distribution
     from http://download.arb-home.de/release/latest/

     Files:
     arb-*.centos5-amd64.tgz
     arb-*.centos6-amd64.tgz
     arb-*.osx.tgz
     arb-*.ubuntu1004-amd64.tgz
     arb-*.ubuntu1204-amd64.tgz
     arb-*.ubuntu1304-amd64.tgz

     (where '*' stands for the most recent version number, e.g. '6.0.1')

     If your distribution is neither Ubuntu nor Centos, try Centos. 
     If your distributions version is not among the above, try the next older
     release (e.g. 12.04 if you have 12.10).

     Notes:
        * All versions are compiled without OPENGL.
          (if you rely on OPENGL features, either stick with arb_5.5 or 
          compile arb from source)
        * Debian users need to pick Centos (due to fun with [e]glibc) or better
          use the package from non-free.
         
  2a) Install using "arb_install.sh"

      Open a terminal, change to the directory where you have saved the
      files needed for installation:

         'arb_install.sh'                // install script
         'arb_README.txt'                // this file
         'arb-*.tgz'                     // your chosen ARB version

      and type

          sh arb_install.sh

      (to install arb for all users call the script as root or via sudo)

      Answer all questions asked by the script.

      Notes:
         - The script will ask about a pt_server directory. This is a
           directory where arb will store big index files.
           You should enter a different path as you do not want to
           recreate those files after an ARB update.
         - Normally pressing enter will be a good choice.
         - You can rerun the script many times, it can be used to
            change an existing installation.

      Change your ".cshrc" or ".profile" files:

      Set the environment variable ARBHOME to the ARB installation directory
      Append   $ARBHOME/bin   to your PATH.
      Re-Login to load the changed environment.

      Go to a directory with a demo database eg 'demo.arb'and start 'ARB' with
        
         arb

  2b) Install manually

      i)   Create a folder where you want ARB to reside. E.g:
      
         mkdir ~/arb-6.0

      ii)  Unpack ARB into this folder. E.g.

         tar -C ~/arb-6.0 -xzf ~/Downloads/arb-6.0.1.ubuntu1304-amd64.tgz

      iii) Install library dependencies. 
 
      (If you already have an ARB installation and just want to install another
      version, you can skip this step.)

      ARB needs:
         gnuplot, gv, openmotif, xfig, transfig, xterm, libtiff, libX11, libXaw, 
         libXext, libxml2, libXpm, libXt, lynx, time, libxslt, glib2, readline

      On Centos, dependencies should be satisfied by running:
        
         sudo yum install gnuplot openmotif xfig transfig xterm libXpm \
            libtiff libX11 libXaw libXext libxml2 libXpml libXt lynx \
            time libxslt readline glib2

      On Ubuntu 13.04, use:

         sudo apt-get install gnuplot gv libmotif3 xfig transfig \
           xterm libxpm4 libtiff4 libx11-6 libxaw7 libxext6 libxml2-utils \
           libxt6 lynx libreadline6
         
      If any of the packages is not available on your system, it might just
      have a slightly differing name. Try using  "yum search" or 
      "apt-cache search" to  search for packages matching the base name 
      (e.g. "yum search libxpm") and pick the one that's available. The only
      known issue is that you must not use "lesstif" instead of 
      motif/openmotif (if you do, the interface will look broken).

      You can now run ARB simply by entering "./bin/arb" from the folder
      where you installed ARB. For convenience, put a symlink to the file
      into any folder that is in your PATH so that you can just type "arb"
      to run it.


Compiling ARB from source
-------------------------

  * See arb_INSTALL.txt for information about how to compile
    arb from source.


Additional information
----------------------

  * See arb_INSTALL.txt for additional software needed and/or useful
   together with ARB.


PT_server
---------

  To Install 'ARB' you have to know that some modules use a so
  called 'pt_server' (prefix tree server).

  ARB needs a writeable directory to store the pt_server files.
  Those files are needed for fast database search
  (by probe_design, probe_match and the automatic aligner)
  and need a lot of disc space (up to several 100 mega bytes,
  e.g. 4000 16S RNA sequences require about 40 MB).
  This files are not created now. They can be build by any user via
      <ARB_NTREE/Probes/PT_SERVER admin/Build server>
  You may define a special directory as the pt_server files location.
  This prevents any loss of data installing a new version of
  ARB.
  To minimize the use of resources in a workstation cluster
  only one pt_server for each database is started on a user defined
  host. The first user starts the pt_server, and all other users
  can connect to it.


The Database
------------

  In the current release a small dataset (demo.arb) is provided.
  This database contains a selection of artificial and real-life
  sequences.

  The intention of providing this small dataset first is to give
  you the opportunity to get familiar with the program and to
  test the performance of your computer system as well as the
  stability of ARB on your system.


Bugs
----

  ARB is running properly and stably on our systems. However, it
  may be that there are bugs never detected by us or never
  appearing on our systems. Please don't hesitate to inform us
  about any bugs. A detailed description of the steps performed
  before the problem was evident and of the number and types of
  modules running at the same time would be extremely helpful to
  our computer scientists.

  Please report bugs into our bug tracker at
         http://bugs.arb-home.de/


Support
-------

  Please send any comments, bug reports or questions to

         arb@arb-home.de


Copyright Notice
----------------

   Please see the file

         arb_LICENSE.txt

  in the ARB installation directory.


Disclaimer
----------

  THE AUTHORS OF ARB GIVE NO WARRANTIES, EXPRESSED OR IMPLIED
  FOR THE SOFTWARE AND DOCUMENTATION PROVIDED, INCLUDING, BUT
  NOT LIMITED TO WARRANTY OF MERCHANTABILITY AND WARRANTY OF
  FITNESS FOR A PARTICULAR PURPOSE.


Have fun!


