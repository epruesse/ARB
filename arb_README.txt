
Welcome to the 'ARB' Sequence Database Tools

/*********** Hardware and System Requirements *************/

	ARB is currently developed on SUN workstations and Linux PCs.
	The most recent version is now available for this machines.

	Release dates / history:

		HP Series 7000	June 95
		PC Linux	Jan  96 ( 486dx; >16 Mega Byte RAM)
		SGI Irix	June 96
		Digital OSF	April97
		SUNOS4.x	Mai  92
		SUNOS5.x	June 94

	Hardware Requirements:
				Minimum		Good
	Real Memory		64		128-256
	Free Disc Space		100		1000
	Computer Speed		486dx66		586dx90
				Sparc 1		Sparc 5/10/20

	Note:	Memory is more important than a fast processor, a 486dx
		width 64 mByte of RAM may be much faster than an
		Ultra Sparc with 32 mByte of RAM.


/*********** Files needed to install ARB *************/

	File		FTP server location		// Comment

	'arb_README.txt'	pub/ARB/		// this file
	'arb_install.sh'	pub/ARB/$MACH		// install script
	'arb.tgz'		pub/ARB/$MACH		// ARB program
	'zcat'			pub/ARB/$MACH		// decompress (gzip)
        ['arb_ale.tgz'		pub/ARB/$MACH		// optional Editor ]
        ['***.arb'		pub/ARB/data/		// optional demo /
      								   real rRNA data ]

	Notes:
		- $MACH	should be replaced by your system type
			( type uname -sr to find out your system type )
		- enable binary mode for ftp transfer ( command 'bin' )
		- do not uncompress and untar arb.tar.gz directly, use the
			install script !!!

/*********** Install/Update ARB *************/

	ARB consists of more than 750 files which are installed into a single
	directory. Creating this directory, copying all data into it, and setting
	the permissions correctly is done by the installation script
		'arb_install'

	Goto the directory, where the files

		'arb_install.sh'		//install script
		'arb_README.txt'		//this file
		'arb.tgz'			//all the libs and bin
		'zcat'				//decompress
	      [	'arb_ale.tgz'			//optional sequence editor ]

	are located and type '/bin/sh arb_install.sh'

	Answer all questions asked by the script.
	Notes: -The script will ask about a pt_server directory. This is a
		directory where arb will store big index files.
		You should enter a different path as you do not want to
		recreate those files after an ARB update.
	      - Normally pressing enter will be a good choice.
	      - You can rerun the script many times, it can be used to
		change an existing installation.

	Change your .cshrc/.profile files:
		Set the enviroment variable ARBHOME
		to the ARB installation directory
		Append   $ARBHOME/bin   to your PATH

	reread it,	(logout+login )

	goto a directory with a demo database 'eg demo.arb'
	and start 'ARB' with

			'arb'

/*********** The next pages are for experts only *************/
/*********** (or people who have already installed phylogenetic software) *************/

/*********** External (unsupported) programs, which are part of the ARB distribution *************/

	The programs of ARB use foreign software. This includes:

	1. Basic Sequence Analysis Tools:
		PHYLIP, fasta, blast, zuker, fastDNAml, ...
	2. GUI Supporting Sequence Analysis Tools:
		GDE, treetool, ...
	3. Public Domain X11 Software:
		xfig, fig2dev, ghostview, ...
	4. Some non standard X11 Shared Libraries
		(see next topic)
	5. emacs & ale

	It may happen that we don't supply the most recent versions of the
	foreign software.
	It also may happen that you already have installed some of these
	programs. In such a case it may happen that

	1.	the old version is not compatible with the new one.
	2.	you waste disc space.

/*********** Libraries on Solaris 1.x Computers *************/

ARB uses  several non standard libraries:

	1. Motif (shared library or statically linked)
	2. libXt.so.4.10			newer Version of the Xt lib
	3. libX11.so.4.10			newer X11 Version
	4. libXaw.so.5.0
	4. libXext.so.4.10
	4. libXmu.so.4.10
	5. /usr/lib/X11/XKeySymDB		key mappings

	6. libAWT.so.2.0			ARB exTendet Widget Set
	7. libAW.so.2.0				ARB Widget Set
	8. libARBDB.so.2.0			ARB Database lib
	9. libARBDBPP.so.2.0			ARB Object Oriented Database lib

	In a case you have already installed X11 R5/6 remove the duplicated
	shared library in $ARBHOME/lib.


/*********** LINUX for PC *************/
	If you use Linux you should have these packages installed:

	xfig			// simple drawing programm
	transfig		// used to print trees
	gs and ghostview	// previewing trees
	complete xview		// for gde
	X11			// because ARB is based on X11

	Maybe you have a library problem. You can check this after installing
	ARB by typing 'ldd ${ARBHOME}/bin/arb_ntree'
		(Replace ${ARBHOME} by your ARB installation directory;


/*********** PT_server *************/

	To Install 'ARB' you have to know that some modules use a so
	called 'pt_server' (prefix tree server).

	ARB needs a writeable directory to store the pt_server files.
	Those files are needed for fast database search
	(by probe_design, probe_match and the automatic aligner)
	and need a lot of disc space (up to several 100 mega bytes,
	e.g. 4000 16S RNA sequences require about 40 MB).
	This files are not created now, but later on by all users
		<ARB_NT/etc/PT_SERVER admin/update server>
	You may define a special directory as the pt_server files location.
	This prevents any loss of data installing a new version of
	ARB.
	To minimize the use of ressources in a workstation cluster
	only one pt_server for each database is started on a user defined
	host. The first user starts the pt_server, and all other users
	can connect to it.

/*********** What you should know:	Swap *************/

	Arb needs a lot of virtual memory
	(about 50 Mbyte for 5000 Sequences, Length = 3000).
	You can find out about installed swap space by typing:
		SunOS 1.x:		'pstat -s'
		System V / SGI		'swap -s'
		OSF			'swapon -sv'
		Linux			'top'

	Call your system administrator or local guru to increase your
	swap. (If you don't have such a nice person, try to read the
	man pages: 'man -k swap')

/*********** Install sub programs of ARB *************/

	If you want to use the blast/fasta database search tools, read the
	file $ARBHOME/GDEHELP/GDE2.2_manual_text how to install the databases.
	Note:	The GDE software incorporated into ARB does not use the
		enviroment variable
			$GDE_HELP_DIR
			but $ARBHOME/GDEHELP
		and neither the menu description file
			$GDE_HELP_DIR/.GDEmenus
			but $ARBHOME/GDEHELP/ARB_GDEmenus.

/************* The Database ************/

	In the current release a small dataset of about 140 16S rRNA sequences
	(6demo.arb) is provided. This database contains a selection of the
	(most) complete sequences covering the majority of the major lines of
	descent of Bacteria and Archaea.

	The intention of providing this small dataset first is to give you the
	opportunity to get familiar with the program and to test the performance
	of your computer system as well as the stability of ARB on your system.

	ARB is running properly and stably on our systems. However, it may be
	that there are bugs never detected by us or never appearing on our
	systems. Please don't hesitate to inform us about any bugs. A detailed
	description of the steps performed before the problem was evident and
	of the number and types of modules running at the same time would
	extremely be helpful to our computer scientists.

	The 6demo.arb database contains a tree (tree_demo) that comprises all
	species from which data are included. This tree is based on a (genreally
	optimized) parsimony analysis. Be aware, that parsimony is not the
	optimum method for treeing (is there any??). The major purpose of the
	tree is to give you the oportunity to walk through the database by using
	the mouse. To reconstruct better (sub) trees, use other integrated
	treeing methods (PHYLIP, De Soete, fastDNAml, ARB neighourjoining;
	see online help).

	In its default version, the tree shows groups. There is a completely
	ungrouped (tree_ungrouped) version included too as well as subtrees for
	the Bacteria and Archaea. See the online help texts to handle and modify
	trees and data. Please let us know whether online help texts are
	sufficiently informative. The help texts will permanentely be updated.
	Check our FTP server from time to time for new releases.

	To import your data, preferrably use the CREATE AND IMPORT tool (if
	the formate is consistent (see online help), create a new database
	(*.arb) and merge it with the existing if desired. If your formate does
	not match one of those detected by the 	CREATE AND IMPORT tool, try to
	use the IMPORT FOREIGN FORMATE tool of the integrated GDE software.
	Finally, you may create a new species entry and simply copy your
	sequence text to the INFO window (see on line help).

	If your alignment is different from ours, you have to adopt the HELIX
	information (see online help) to use the secondary checking facility.

	If you want to test the automated aligner or the probe design tool, you
	have to export the database to the PT SERVER (see online help).

	Ther are three examples of masks (SAI entries): arc5r, bac5r, gpl5rr
	which were calculated for Archaea, Bacteria, and Gram-positives with a
	low G+C, respectively, and define those positions which are identical in
	at least 50% of all sequences of the particular groups in our database.
	positions at which gap symbols represent the majority are defined as to
	exclude also.

	A comprehensive dataset of about 3500 16S sequences will be provided
	during the next days. Check the FTP server or your mail box.




/*********** Support *************/

	Please send any comments, bug reports  or questions to

		arb@arb-home.de

/*********** Copyright Notice *************/

	The ARB software and documentation are not in the public domain.
	External functions used by ARB are the proporty of, their respective
	authors. This release of the ARB program and documentation may not
	be sold, or incorporated into a commercial product, in whole or in
	part without the expressed written consent of the Technical
	University of Munich and of its supervisors
	Oliver Strunk & Wolfgang Ludwig.

	All interested parties may redistribute the ARB as
	long as all copies are accompanied by this
	documentation,  and all copyright notices remain
	intact.  Parties interested in redistribution must do
	so on a non-profit basis, charging only for cost of
	media.

/*********** Disclaimer *************/

	THE AUTHORS OF ARB GIVE NO WARRANTIES,
	EXPRESSED OR IMPLIED FOR THE SOFTWARE AND
	DOCUMENTATION PROVIDED, INCLUDING,
	BUT NOT LIMITED TO WARRANTY OF
	MERCHANTABILITY AND WARRANTY OF
	FITNESS FOR A PARTICULAR PURPOSE.

