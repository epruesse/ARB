
Welcome to the 'ARB' Sequence Database Tools


System Requirements
------------------- 


        ARB is currently developed for Linux and OSX.

        Hardware Requirements (Linux):

                                Good                    We use
        Real memory :           1 Gb                    4 Gb    
        Free discspace :        1 Gb                    more    
        Computer speed :        2 GHz Cpu               2 GHz Dual Cpu
                                

        Note : Memory is more important than a fast processor!


Files needed to install ARB
---------------------------

        File                    

                'arb_install.sh'        // install script
                'arb-*.tgz'             // ARB program


        Available ARB versions:

                arb-*.centos5-amd64.tgz
                arb-*.centos6-amd64.tgz
                arb-*.ubuntu1004-amd64.tgz
                arb-*.ubuntu1204-amd64.tgz
                arb-*.ubuntu1304-amd64.tgz
                arb-*.osx.tgz

        You need exactly ONE of the available 'arb-*.tgz'!

        Notes:

        * all versions are 64bit versions
          (if you really need a 32bit version, see http://download.arb-home.de/release/arb_5.5/)

        * all versions are compile without OPENGL
          (if you rely on OPENGL features, either stick with arb_5.5 or compile arb from source)

        * OSX version is available as arb-*.osx.tgz
          and via MacPorts (see arb_OS_X_MacPorts.txt included inside arb-*.osx.tgz)


Install/Update ARB
------------------

        ARB consists of more than 1400 files which are installed into a single
        directory. Creating this directory, copying all data into it, and setting
        the permissions correctly is done by the installation script

                'arb_install.sh'

        Open a terminal, change to the directory where you have saved the
        files needed for installation:

                'arb_install.sh'                // install script
                'arb_README.txt'                // this file
                'arb-*.tgz'                     // your chosen ARB version


        and type

                '/bin/sh arb_install.sh'

        (to install arb for all users call the script as root or via sudo)


        Answer all questions asked by the script.

        Notes:- The script will ask about a pt_server directory. This is a
                directory where arb will store big index files.
                You should enter a different path as you do not want to
                recreate those files after an ARB update.
              - Normally pressing enter will be a good choice.
              - You can rerun the script many times, it can be used to
                change an existing installation.

        Change your .cshrc/.profile files:

                Set the environment variable ARBHOME
                to the ARB installation directory
                Append   $ARBHOME/bin   to your PATH

        reread it,      (logout+login)

        goto a directory with a demo database 'eg demo.arb'
        and start 'ARB' with

                        'arb'


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
        To minimize the use of ressources in a workstation cluster
        only one pt_server for each database is started on a user defined
        host. The first user starts the pt_server, and all other users
        can connect to it.


What you should know: Swap
--------------------------

        Arb needs a lot of virtual memory
        (about 50 Mbyte for 5000 Sequences, Length = 3000).
        You can find out about installed swap space by typing:

                Linux                   'top'
                SunOS 1.x:              'pstat -s'
                System V / SGI          'swap -s'
                OSF                     'swapon -sv'

        Call your system administrator or local guru to increase your
        swap. (If you don't have such a nice person, try to read the
        man pages: 'man -k swap')


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


