
#********************* Start of user defined Section

# To setup your computer, change file config.makefile first


#	set ARBHOME to this directory
( export ARBHOME = `pwd` )

#	disable all lib dirs
LD_LIBRARY_PATH = ${SYSTEM_LD_LIBRARY_PATH}:$(ARBHOME)/LIBLINK:$(ARBHOME)/lib

# get the machine type
include config.makefile

#********************* Default set and gcc static enviroments *****************

FORCEMASK = umask 002

#---------------------- Some compiler-specific defaults

ifdef ECGS
dflag1 = -ggdb3
enumequiv =
havebool = -DHAVE_BOOL
else
dflag1 = -g
enumequiv = -fenum-int-equiv
havebool =
endif

#----------------------

ifeq ($(DEBUG),1)
ifeq ($(DEBUG_GRAPHICS),1)
		dflags = -DDEBUG -DDEBUG_GRAPHICS
else
		dflags = -DDEBUG
endif
		cflags = $(dflag1) $(dflags)
		lflags = $(dflag1)
		fflags = $(dflag1) -C
		extended_warnings = -Wwrite-strings -Wunused -Wno-aggregate-return
		extended_cpp_warnings = -Wnon-virtual-dtor -Wreorder
else
ifeq ($(DEBUG),0)
		dflags = -DNDEBUG
		cflags = -O $(dflags)
		lflags = -O
		fflags = -O
		extended_warnings =
		extended_cpp_warnings =
endif
endif

   PREFIX =
   LIBDIR = /usr/lib
   XHOME = /usr/X11

   GMAKE = gmake -r
   CPP = g++ -W -Wall $(enumequiv) -D$(MACH) $(havebool) -pipe#		# C++ Compiler /Linker
   PP = cpp
   ACC = gcc -Wall -fenum-int-equiv -D$(MACH) -pipe#				# Ansi C
   CCLIB = $(ACC)#			# Ansii C. for shared libraries
   CCPLIB = $(CPP)#			# Same for c++
   AR = ld -r -o#			# Archive Linker
   ARLIB = ld -r -o#			# The same for shared libs.
   XAR = $(AR)# 			# Linker for archives containing templates
   MAKEDEPEND = $(FORCEMASK);makedepend $(dflags)
   SHARED_LIB_SUFFIX = so#		# shared lib suffix
   F77 = f77

   CTAGS = etags
   CLEAN_BEFORE_MAKE = # make clean before all (needed because of bug in Sun CC with templates)
   XMKMF = 	/usr/bin/X11/xmkmf

ifdef SEER
    SEERLIB = SEER/SEER.a
else
    SEERLIB =
endif

#********************* Darwin gcc enviroments *****************
ifdef DARWIN

   XHOME = /usr/X11R6
   havebool = -DHAVE_BOOL
   lflags = $(LDFLAGS) -force_flat_namespace
   DARWIN_SPECIALS = -DNO_REGEXPR  -no-cpp-precomp -DHAVE_BOOL
   CPP = cc -D$(MACH) $(DARWIN_SPECIALS)
   ACC = cc -D$(MACH) $(DARWIN_SPECIALS)
   CCLIB = cc -fno-common -D$(MACH) $(DARWIN_SPECIALS)
   CCPLIB = cc -fno-common -D$(MACH) $(DARWIN_SPECIALS)
   AR = ld -r $(lflags) -o#			# Archive Linker
   ARLIB = ld -r $(lflags)  -o#			# Archive Linker shared libs.
#  ARLIB = cc -bundle -flat_namespace -undefined suppress -o
   SHARED_LIB_SUFFIX = a#
# .. Just building shared libraries static, i was having problems otherwise

   GMAKE = make -j 3 -r
   SYSLIBS = -lstdc++

   MOTIF_LIBNAME = libXm.3.dylib
   MOTIF_LIBPATH = $(LIBDIR)/$(MOTIF_LIBNAME)
   XINCLUDES = -I/usr/X11R6/include
   XLIBS = -L$(XHOME)/lib $(SYSLIBS) -lXm -lXt -lX11 -lXext -lXp  -lc

   PERLBIN = /usr/bin
   PERLLIB = /usr/lib
   CRYPTLIB = -L/usr/lib -lcrypt

endif

#********************* Linux and gcc enviroments *****************
ifdef LINUX

   LINUX_SPECIALS = -DNO_REGEXPR
   SITE_DEPENDEND_TARGETS = perl
   CPP := $(CPP) $(LINUX_SPECIALS) $(extended_warnings) $(extended_cpp_warnings)
   ACC := $(ACC) $(LINUX_SPECIALS) $(extended_warnings)
   CCLIB = $(ACC) -fpic
   CCPLIB = $(CPP) -fpic	#			# Same for c++
   f77_flags = $(fflags) -W -N9 -e
   F77LIB = -lU77

   ARCPPLIB = g++ -Wall -shared $(LINUX_SPECIALS) -o
   ARLIB = gcc -Wall -shared $(LINUX_SPECIALS) -o
   GMAKE = make -j 3 -r
   SYSLIBS = -lm
ifndef DEBIAN
   XINCLUDES = -I/usr/X11/include -I/usr/X11/include/Xm -I/usr/openwin/include
   XLIBS = -lXm -lXpm -lXp -lXt -lXext -lX11 -L$(XHOME)/lib $(SYSLIBS) -lc
else
   XINCLUDES = -I/usr/X11R6/include
   XLIBS = -L/usr/X11R6/lib -lXm -lXpm -lXp -lXt -lXext -lX11 -L$(XHOME)/lib $(SYSLIBS) -lc
endif

   OWLIBS =  -L${OPENWINHOME}/lib -lxview -lolgx -L$(XHOME)/lib -lX11 $(SYSLIBS) -lc
   PERLBIN = /usr/bin
   PERLLIB = /usr/lib
   CRYPTLIB = -L/usr/lib -lcrypt

endif

#********************* SUN4 && acc CC enviroments *****************
#********************* SUN4 dynamic libraries libC *****************

ifdef SUN4
   SITE_DEPENDEND_TARGETS = perl
   ARLIB =	ld -assert pure-text -o
   CPP = CC -D$(MACH) -DNO_REGEXPR
   PP = cc -D$(MACH) -E
   ACC = acc -D$(MACH) -DNO_REGEXPR
   CCPLIB = $(CPP) -pic
   CCLIB = $(ACC) -pic

   XMKMF = 	/usr/openwin/bin/xmkmf
   SHARED_LIB_SUFFIX = so.2.0

   XINCLUDES = -I$(XHOME)/include -I$(OPENWINHOME)/include

   STATIC = -Bstatic
   DYNAMIC = -Bdynamic
   MOTIFLIB =  -lXm
   SYSLIBS = -lm
   XLIBS =  -L$(XHOME)/lib $(MOTIFLIB) -lXt -lX11 $(SYSLIBS) $(CCPLIBS)
   OWLIBS =  -L$(OPENWINHOME)/lib -lxview -lolgx -lX11 $(SYSLIBS)

endif


#********************* SUN5  CC enviroments  *****************
#********************* SUN5  ****
ifdef SUN5
   SITE_DEPENDEND_TARGETS = perl

ifdef ECGS
   CPP = g++ -W -Wall $(enumequiv) -D$(MACH) -D$(MACH)_ECGS $(havebool) -pipe#		# C++ Compiler /Linker
   ACC = gcc -Wall -fenum-int-equiv -D$(MACH) -D$(MACH)_ECGS -pipe#				# Ansi C

else
   #AR = $(FORCEMASK);ld -r -o#			# Archive Linker
   AR = $(FORCEMASK);CC -xar -o#
   XAR = $(FORCEMASK);CC -xar -o#
   ARLIB = $(FORCEMASK);ld -G -o#

ifdef SUN_WS_50

   # fake pointer to virtual table at start of structs (when passing classes to C)
   FAKE_VIRTUAL_TABLE_POINTER = -DFAKE_VIRTUAL_TABLE_POINTER=char

   havebool = -DHAVE_BOOL

   SUN_ACC_FLAGS = -errtags=yes -erroff=E_MODIFY_TYPEDEF_IGNORED $(havebool) $(FAKE_VIRTUAL_TABLE_POINTER)
   SUN_CPP_FLAGS = +w2 $(havebool) $(FAKE_VIRTUAL_TABLE_POINTER)
   AR = ld -r -o#
   ARLIB = CC -G -o#
else
   SUN_ACC_FLAGS =
   SUN_CPP_FLAGS = +w2
endif

   ACC = $(FORCEMASK);cc -D$(MACH) $(SUN_ACC_FLAGS)
   CPP = $(FORCEMASK);CC -D$(MACH) $(SUN_CPP_FLAGS)
   PP = $(FORCEMASK);cc -D$(MACH) -E
   CCLIB = cc -D$(MACH) $(SUN_ACC_FLAGS) -Kpic
   CCPLIB = CC -D$(MACH) $(SUN_CPP_FLAGS) -PIC

   XHOME = /usr/dt
   XMKMF = 	/usr/openwin/bin/xmkmf
   f77_flags = $(fflags) -e -silent
   F77LIB = -nolib -Bstatic -lF77 -lsunmath -Bdynamic -lm -lc

   XINCLUDES = -I$(XHOME)/include -I$(OPENWINHOME)/include
   STATIC = -Bstatic
   DYNAMIC = -Bdynamic

   SYSLIBS = -lm -lsocket -lnsl -lgen -lposix4
   XLIBS =  -L$(OPENWINHOME)/lib -L$(XHOME)/lib -lXm -lXt -lX11 $(SYSLIBS)
   OWLIBS =  -L$(OPENWINHOME)/lib -lxview -lolgx -lX11 -L/usr/ucblib -lucb $(SYSLIBS)
   CTAGS = etags
   CLEAN_BEFORE_MAKE = $(MAKE) clean# rebuild templates! (needed because of bug in Sun CC)

ifeq ($(DEBUG),1)
	MAKE_RTC = rtc_patch
	RTC = -lRTC8M
endif
endif
endif


#********************* HP and CC/cc enviroments (dynamic) *****************

ifdef HPCC
   ARLIB =	ld -b -o
   HPSPECIALS = -D$(MACH) -DNO_REGEXPR -DNO_INLINE
   XMKMF = /usr/local/bin/X11/xmkmf

   CPP = LDOPTS='+s'; export LDOPTS;CC $(HPSPECIALS)
   ACC = LDOPTS='+s'; export LDOPTS;cc $(HPSPECIALS) -Ae

   CCPLIB = $(CPP) +z
   CCLIB = $(ACC) +z

   SYSLIBS = -codelibs -lm
   SHARED_LIB_SUFFIX = sl

   XINCLUDES = -I/usr/include/X11R5 -I/usr/include/Motif1.2
   XLIBS = -L/usr/lib/X11R5 -L/usr/lib/Motif1.2  -lXm -lXt -lX11 $(SYSLIBS)
endif

#********************* HP and CC/cc enviroments (dynamic) *****************

ifdef DIGITAL
   ARLIB =	ld -r -g -o
   DIGSPECIALS = -D$(MACH) -DNO_REGEXPR
   CPP = cxx -w0 -x cxx $(DIGSPECIALS)
   ACC = cc -w0 $(DIGSPECIALS)

   CCPLIB = $(CPP)
   CCLIB = $(ACC)

   SHARED_LIB_SUFFIX = so
   SYSLIBS =

   STATIC = -non_shared
   DYNAMIC =

   XINCLUDES =
   SYSLIBS = -lm
   XLIBS =  -lXm -lXt -lX11 $(SYSLIBS)
endif

#********************* SGI and CC/cc enviroments (dynamic) *****************

ifdef SGI
   ARLIB =	CC -D$(MACH) -shared -o
   SGISPECIALS	= -DNO_REGEXPR
   CPP =	CC -D$(MACH) $(SGISPECIALS)
   ACC =	cc -w -D$(MACH) $(SGISPECIALS)
   XMKMF = 	/usr/bin/X11/xmkmf
   CCPLIB = $(CPP)
   CCLIB = $(ACC)
   XINCLUDES =
   SYSLIBS = -lm
   XLIBS = -lXm -lXt -lX11 $(SYSLIBS)
endif

#********************* Main dependences *******************
ifndef ARCPPLIB
	ARCPPLIB = $(ARLIB)
endif

SEP=--------------------------------------------------------------------------------

dummy:
		$(MAKE) tests
		@echo $(SEP)
		@echo 'Main targets:'
		@echo ''
		@echo ' all         - Compile ARB + TOOLs + and copy shared libs + link foreign software'
		@echo '               (That is most likely the target you want)'
		@echo ''
		@echo ' clean       - remove intermediate files (objs,libs,etc.)'
		@echo ' realclean   - remove ALL generated files'
		@echo ' rebuild     - realclean + all'
		@echo ''
		@echo 'Some often used sub targets (make all makes them all):'
		@echo ''
		@echo ' arb         - Just compile ARB (but none of the integrated tools)'
		@echo ' menus       - create GDEHELP/ARB_GDEmenus from GDEHELP/ARB_GDEmenus.source'
		@echo ' perl        - Compile the PERL XSUBS into lib/ARB.so  and create links in lib to perl'
		@echo ' binlink     - Create all links in the bin directory'
		@echo ''
		@echo 'Development targets:'
		@echo ''
		@echo ' depend      - create dependencies               (recommended for people not using our CVS)'
		@echo ' XXX/.depend - create dependencies for dir XXX   (recommended for ARB developers)'
		@echo ' tags        - create tags for xemacs'
		@echo ' rmbak       - remove all "*%" and cores'
		@echo ''
		@echo 'Internal maintainance:'
		@echo ''
		@echo ' tarfile     - make rebuild and create "arb.tar.gz" (tarfile_ignore to skip rebuild)'
		@echo ' tarale      - compress emacs and ale lisp files int arb_ale.tar.gz'
		@echo ' save        - save all basic ARB sources into arbsrc_DATE (BROKEN!)'
		@echo ' savedepot   - save all extended ARB source (DEPOT2 subdir) into arbdepot_DATE.cpio.gz'
		@echo ' rtc_patch   - create LIBLINK/libRTC8M.so (SOLARIS ONLY)'
ifeq ($(USER),westram)
		@echo ' export      - make tarfile and export to homepage'
endif
		@echo ''
		@echo $(SEP)
		@echo ''

#********************* End of user defined Section *******************

test_DEBUG:
ifndef dflags
		@echo DEBUG has to be defined. Valid values are 0 and 1
		@echo Please check your config.makefile
		@false
else
		@true
endif

tests: test_DEBUG
		@echo Tests successful

# end test section

DIR = $(ARBHOME)
LIBS = -lARBDB
GUI_LIBS = $(LIBS) -lAW -lAWT $(RTC) $(XLIBS)
LIBPATH = -LLIBLINK

DEST_LIB = lib
DEST_BIN = bin

AINCLUDES = 	-I. -I$(DIR)/INCLUDE $(XINCLUDES)
CPPINCLUDES =	-I. -I$(DIR)/INCLUDE $(XINCLUDES)
MAKEDEPENDINC = -I. -I$(DIR)/DUMMYINC -I$(DIR)/INCLUDE

#*****		List of all Directories
ARCHS = \
			AISC/dummy.a \
			AISC_MKPTPS/dummy.a \
			ALEIO/ALEIO.a \
			ALIV3/ALIV3.a \
			ARBDB/libARBDB.a \
			ARBDB2/libARBDB.a \
			ARBDBPP/libARBDBPP.a \
			ARBDBS/libARBDB.a \
			ARBDB_COMPRESS/ARBDB_COMPRESS.a \
			ARB_GDE/ARB_GDE.a \
			AWDEMO/AWDEMO.a \
			AWT/libAWT.a \
			AWTC/AWTC.a \
			AWTI/AWTI.a \
			CAT/CAT.a \
			CHIP/CHIP.a \
			CONSENSUS_TREE/CONSENSUS_TREE.a \
			CONVERTALN/CONVERTALN.a \
			DBSERVER/DBSERVER.a \
			DIST/DIST.a \
			EDIT/EDIT.a \
			EDIT4/EDIT4.a \
			EISPACK/EISPACK.a \
			GDE/GDE.a \
			GENOM/GENOM.a \
			ISLAND_HOPPING/ISLAND_HOPPING.a \
			MERGE/MERGE.a \
			MULTI_PROBE/MULTI_PROBE.a \
			NALIGNER/NALIGNER.a \
			NAMES/NAMES.a \
			NAMES_COM/server.a \
			NTREE/NTREE.a \
			ORS_CGI/ORS_CGI.a \
			ORS_COM/server.a \
			ORS_SERVER/ORS_SERVER.a \
			PARSIMONY/PARSIMONY.a \
			PHYLO/PHYLO.a \
			PRIMER_DESIGN/PRIMER_DESIGN.a \
			PROBE/PROBE.a \
			PROBE_COM/server.a \
			PROBE_DESIGN/PROBE_DESIGN.a \
			PROBE_GROUP/PROBE_GROUP.a \
			PROBE_SET/PROBE_SET.a \
			READSEQ/READSEQ.a \
			SECEDIT/SECEDIT.a \
			SEER/SEER.a \
			SERVERCNTRL/SERVERCNTRL.a \
			STAT/STAT.a \
			TEST/TEST.a \
			TOOLS/TOOLS.a \
			TREEGEN/TREEGEN.a \
			TRS/TRS.a \
			WETC/WETC.a \
			WINDOW/libAW.a \
			XML/XML.a \


ARCHS_CLIENTACC = PROBE_COM/client.a
ARCHS_CLIENTCPP = NAMES_COM/client.a
ARCHS_CLIENT = $(ARCHS_CLIENTCPP)
ARCHS_MAKEBIN =		AISC_MKPTPS/dummy.a AISC/dummy.a

ARCHS_COMMUNICATION =	NAMES_COM/server.a\
			PROBE_COM/server.a\
			ORS_COM/server.a

# communication libs need aisc and aisc_mkpts:
$(ARCHS_COMMUNICATION:.a=.dummy) : $(ARCHS_MAKEBIN:.a=.dummy)

#***************************************************************************************
#		Individual Programs Section
#***************************************************************************************

#***********************************	arb_ntree **************************************
NTREE = bin/arb_ntree
ARCHS_NTREE = \
		$(ARCHS_CLIENTACC) \
		$(SEERLIB) \
		ARB_GDE/ARB_GDE.a \
		AWTC/AWTC.a \
		AWTI/AWTI.a \
		CAT/CAT.a \
		GENOM/GENOM.a \
		ISLAND_HOPPING/ISLAND_HOPPING.a \
		MERGE/MERGE.a \
		MULTI_PROBE/MULTI_PROBE.a \
		NTREE/NTREE.a \
		PRIMER_DESIGN/PRIMER_DESIGN.a \
		PROBE_DESIGN/PROBE_DESIGN.a \
		SERVERCNTRL/SERVERCNTRL.a \
		STAT/STAT.a \
		XML/XML.a \

$(NTREE): $(ARCHS_NTREE:.a=.dummy) NAMES_COM/server.dummy # aw db awt awtc awti
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_NTREE) $(GUI_LIBS)

#***********************************	arb_edit **************************************
EDIT = bin/arb_edit
ARCHS_EDIT = EDIT/EDIT.a ARB_GDE/ARB_GDE.a STAT/STAT.a XML/XML.a
$(EDIT): $(ARCHS_EDIT:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_EDIT) -lARBDBPP $(GUI_LIBS)

#***********************************	arb_edit4 **************************************
EDIT4 = bin/arb_edit4
ARCHS_EDIT4 = NAMES_COM/client.a AWTC/AWTC.a EDIT4/EDIT4.a SECEDIT/SECEDIT.a \
	SERVERCNTRL/SERVERCNTRL.a STAT/STAT.a ARB_GDE/ARB_GDE.a \
		ISLAND_HOPPING/ISLAND_HOPPING.a XML/XML.a
$(EDIT4): $(ARCHS_EDIT4:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_EDIT4) $(GUI_LIBS)

#***********************************	arb_wetc **************************************
WETC = bin/arb_wetc
ARCHS_WETC = WETC/WETC.a XML/XML.a
$(WETC): $(ARCHS_WETC:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_WETC) $(GUI_LIBS)

#***********************************	arb_dist **************************************
DIST = bin/arb_dist
ARCHS_DIST = DIST/DIST.a SERVERCNTRL/SERVERCNTRL.a CONSENSUS_TREE/CONSENSUS_TREE.a \
		EISPACK/EISPACK.a  XML/XML.a
#		FINDCORRWIN/FINDCORRWIN.a FINDCORRMATH/FINDCORRMATH.a
$(DIST): $(ARCHS_DIST:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_DIST) $(ARCHS_CLIENT) $(GUI_LIBS)

#***********************************	arb_pars **************************************
PARSIMONY = bin/arb_pars
ARCHS_PARSIMONY =	PARSIMONY/PARSIMONY.a  XML/XML.a
$(PARSIMONY): $(ARCHS_PARSIMONY:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PARSIMONY) $(GUI_LIBS)

#*********************************** arb_treegen **************************************
TREEGEN = bin/arb_treegen
ARCHS_TREEGEN =	TREEGEN/TREEGEN.a
$(TREEGEN) :  $(ARCHS_TREEGEN:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_TREEGEN)

#***********************************	arb_naligner **************************************
NALIGNER = bin/arb_naligner
ARCHS_NALIGNER = PROBE_COM/server.a NALIGNER/NALIGNER.a SERVERCNTRL/SERVERCNTRL.a
$(NALIGNER): $(ARCHS_NALIGNER:.a=.dummy)
	@echo $(SEP) Link $@
	cp NALIGNER/NALIGNER.com $@
# no LIB_NALIGNER defined: see NALIGNER/Makefile

#***********************************	arb_secedit **************************************
SECEDIT = bin/arb_secedit
ARCHS_SECEDIT = SECEDIT/SECEDIT.a  XML/XML.a
$(SECEDIT):	$(ARCHS_SECEDIT:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(cflags) -o $@ $(LIBPATH) $(ARCHS_SECEDIT) -lAWT $(LIBS)


ARCHS_PROBE_COMM = PROBE_COM/server.a PROBE/PROBE.a
#***********************************	arb_probe_group **************************************
PROBE_GROUP = bin/arb_probe_group
ARCHS_PROBE_GROUP = SERVERCNTRL/SERVERCNTRL.a $(ARCHS_CLIENTACC) PROBE_GROUP/PROBE_GROUP.a
$(PROBE_GROUP):	$(ARCHS_PROBE_GROUP:.a=.dummy) $(ARCHS_PROBE_COMM:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(cflags) -o $@ $(LIBPATH) $(ARCHS_PROBE_GROUP) $(LIBS)

#***********************************	chip **************************************
CHIP = bin/chip
ARCHS_CHIP = SERVERCNTRL/SERVERCNTRL.a $(ARCHS_CLIENTACC) CHIP/CHIP.a
$(CHIP):	$(ARCHS_CHIP:.a=.dummy) $(ARCHS_PROBE_COMM:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(cflags) -o $@ $(LIBPATH) $(ARCHS_CHIP) $(LIBS)

#***********************************	arb_phylo **************************************
PHYLO = bin/arb_phylo
ARCHS_PHYLO = PHYLO/PHYLO.a  XML/XML.a
$(PHYLO): $(ARCHS_PHYLO:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PHYLO) $(GUI_LIBS)


#***************************************************************************************
#					SERVER SECTION
#***************************************************************************************

#***********************************	arb_db_server **************************************
DBSERVER = bin/arb_db_server
ARCHS_DBSERVER = DBSERVER/DBSERVER.a SERVERCNTRL/SERVERCNTRL.a
$(DBSERVER): $(ARCHS_DBSERVER:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_DBSERVER) -lARBDB $(SYSLIBS)

#***********************************	arb_pt_server **************************************
PROBE = bin/arb_pt_server
ARCHS_PROBE = PROBE_COM/server.a PROBE/PROBE.a SERVERCNTRL/SERVERCNTRL.a
$(PROBE): $(ARCHS_PROBE:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) PROBE/PROBE.a PROBE_COM/server.a \
			SERVERCNTRL/SERVERCNTRL.a PROBE_COM/client.a $(STATIC) -lARBDB $(CCPLIBS) $(DYNAMIC) $(SYSLIBS)

#***********************************	arb_name_server **************************************
NAMES = bin/arb_name_server
ARCHS_NAMES = NAMES_COM/server.a NAMES/NAMES.a SERVERCNTRL/SERVERCNTRL.a
$(NAMES): $(ARCHS_NAMES:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) NAMES/NAMES.a SERVERCNTRL/SERVERCNTRL.a NAMES_COM/server.a NAMES_COM/client.a -lARBDB $(SYSLIBS) $(CCPLIBS)

#***********************************	ors **************************************
ORS_SERVER = tb/ors_server
ARCHS_ORS_SERVER = ORS_COM/server.a ORS_SERVER/ORS_SERVER.a SERVERCNTRL/SERVERCNTRL.a
$(ORS_SERVER): $(ARCHS_ORS_SERVER:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) ORS_SERVER/ORS_SERVER.a SERVERCNTRL/SERVERCNTRL.a ORS_COM/server.a ORS_COM/client.a $(STATIC) -lARBDB $(DYNAMIC) $(SYSLIBS) $(CCPLIBS) $(CRYPTLIB)

ORS_CGI = tb/ors_cgi
ARCHS_ORS_CGI = ORS_COM/server.a ORS_CGI/ORS_CGI.a SERVERCNTRL/SERVERCNTRL.a
$(ORS_CGI): $(ARCHS_ORS_CGI:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) ORS_CGI/ORS_CGI.a SERVERCNTRL/SERVERCNTRL.a ORS_COM/client.a $(STATIC) -lARBDB $(DYNAMIC) $(SYSLIBS) $(CCPLIBS)


EDITDB = tb/editDB
ARCHS_EDITDB = EDITDB/EDITDB.a  XML/XML.a
$(EDITDB): $(ARCHS_EDITDB:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(ARCHS_EDITDB) -lARBDB -lAWT $(LIBS)


#***********************************	TEST SECTION  **************************************
AWDEMO = tb/awdemo
ARCHS_AWDEMO = AWDEMO/AWDEMO.a
$(AWDEMO): $(ARCHS_AWDEMO:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(ARCHS_AWDEMO) $(LIBS)

TEST = tb/dbtest
ARCHS_TEST = TEST/TEST.a  XML/XML.a
$(TEST):	$(ARCHS_TEST:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_TEST)  -lAWT $(LIBS)

ALIV3 = tb/aliv3
ARCHS_ALIV3 = PROBE_COM/server.a ALIV3/ALIV3.a SERVERCNTRL/SERVERCNTRL.a
$(ALIV3): $(ARCHS_ALIV3:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) ALIV3/ALIV3.a SERVERCNTRL/SERVERCNTRL.a PROBE_COM/client.a -lARBDB $(SYSLIBS) $(CCPLIBS)


ACORR = tb/acorr
ARCHS_ACORR = 	DIST/DIST.a SERVERCNTRL/SERVERCNTRL.a FINDCORRASC/FINDCORRASC.a FINDCORRMATH/FINDCORRMATH.a FINDCORRWIN/FINDCORRWIN.a  XML/XML.a
$(ACORR): $(ARCHS_ACORR:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_ACORR) $(ARCHS_CLIENT) -lAWT -lARBDBPP $(LIBS)


ARBDB_COMPRESS = tb/arbdb_compress
ARCHS_ARBDB_COMPRESS = ARBDB_COMPRESS/ARBDB_COMPRESS.a
$(ARBDB_COMPRESS): $(ARCHS_ARBDB_COMPRESS:.a=.dummy)
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) ARBDB_COMPRESS/ARBDB_COMPRESS.a -lARBDB


#***************************************************************************************
#			Rekursiv calls to submakefiles
#***************************************************************************************
:
%.depend:
	@$(GMAKE) -C $(@D) -r \
		"LD_LIBRARY_PATH  = ${LD_LIBRARY_PATH}" \
		"MAKEDEPENDINC = $(MAKEDEPENDINC)" \
		"MAKEDEPEND=$(MAKEDEPEND)" \
		"ARBHOME=$(ARBHOME)" \
		depend;
	@(grep "^# DO NOT DELETE" $(@D)/Makefile >/dev/null && cat $(@D)/Makefile \
		| sed	-e "s/\/[^ 	]*\/DUMMYINC\/[^ 	]*\.h//g" \
			-e "s/\/usr\/[^ 	]*\.h//g" \
			-e "s&$(ARBHOME)&\\\$$(ARBHOME)&g" \
			-e '/^[A-Za-z][A-Za-z0-9_]*\.o:[ ]*$$/d' \
		>$(@D)/Makefile.2 && \
		mv $(@D)/Makefile.2 $(@D)/Makefile) || echo nop

#
#			-e "s/^[A-Za-z0-9_]+\.o: *$/\#deleted/g" \
#			-e "s/\/[^ 	]*\/INCLUDE/\\\$$(ARBHOME)\/INCLUDE/g"


%.dummy:
	@echo $(SEP) Making $(@F:.dummy=.a) in $(@D)
	@$(GMAKE) -C $(@D) -r \
		"GMAKE = $(GMAKE)" \
		"ARBHOME = $(ARBHOME)" "cflags = $(cflags) -D_ARB_$(@D:/=)" "lflags = $(lflags)" \
		"CPPINCLUDES = $(CPPINCLUDES)" "AINCLUDES = $(AINCLUDES)" \
		"F77 = $(F77)" "f77_flags = $(f77_flags)" "F77LIB = $(F77LIB)" \
		"CPP = $(CPP)" "ACC = $(ACC)" \
		"CCLIB = $(CCLIB)" "CCPLIB = $(CCPLIB)" "CCPLIBS = $(CCPLIBS)" \
		"AR = $(AR)" "XAR = $(XAR)" "ARLIB = $(ARLIB)" "ARCPPLIB = $(ARCPPLIB)" \
		"LIBPATH = $(LIBPATH)" "SYSLIBS = $(SYSLIBS)" \
		"XHOME = $(XHOME)" "STATIC = $(STATIC)"\
		"SHARED_LIB_SUFFIX = $(SHARED_LIB_SUFFIX)" \
		"LD_LIBRARY_PATH  = $(LD_LIBRARY_PATH)" \
		"CLEAN_BEFORE_MAKE  = $(CLEAN_BEFORE_MAKE)" \
		"MAIN = $(@F:.dummy=.a)"


#***************************************************************************************
#			Short aliases to make targets
#***************************************************************************************

mbin:	$(ARCHS_MAKEBIN:.a=.dummy)

com:	$(ARCHS_COMMUNICATION:.a=.dummy)

help:   xml HELP_SOURCE/dummy.dummy

dball:	db dbs db2
db:		ARBDB/libARBDB.dummy
dbs:	ARBDBS/libARBDB.dummy
db2:	ARBDB2/libARBDB.dummy
dp:		ARBDBPP/libARBDBPP.dummy
aw:		WINDOW/libAW.dummy
awt:	AWT/libAWT.dummy
awtc:	AWTC/AWTC.dummy
awti:	AWTI/AWTI.dummy

island_hopping:	ISLAND_HOPPING/ISLAND_HOPPING.dummy

mp: 	MULTI_PROBE/MULTI_PROBE.dummy
ge: 	GENOM/GENOM.dummy
prd: 	PRIMER_DESIGN/PRIMER_DESIGN.dummy

nt:		$(NTREE)
ed:		$(EDIT)

al:		$(ALIGNER)
nal:	$(NALIGNER)
a3:		$(ALIV3)

di:		$(DIST)
ph:		$(PHYLO)
pa:		$(PARSIMONY)
tg:		$(TREEGEN)
se:		$(SECEDIT)
acc:	$(ACORR)

ds:		$(DBSERVER)
pr:		$(PROBE)
pg:		$(PROBE_GROUP)
chip:	$(CHIP)
pd:		PROBE_DESIGN/PROBE_DESIGN.dummy
na:		$(NAMES)
os:		$(ORS_SERVER)
oc:		$(ORS_CGI)

ac:		$(ARBDB_COMPRESS)

te:		$(TEST)
sec:	$(SECEDIT)
de:		$(AWDEMO)

e4:		$(EDIT4)
we:		$(WETC)
eb:		$(EDITDB)

xml:	XML/XML.dummy

#********************************************************************************

depend: $(ARCHS:.a=.depend)

#********************************************************************************

tags: tags_$(MACH)
tags_LINUX: tags2
tags_SUN5: tags1

tags1:
# first search class definitions
		$(CTAGS)          --language=none "--regex=/^[ \t]*class[ \t]+\([^ \t]+\)/" `find . -name '*.[ch]xx' -type f`
		$(CTAGS) --append --language=none "--regex=/\([^ \t]+\)::/" `find . -name '*.[ch]xx' -type f`
# then append normal tags (headers first)
		$(CTAGS) --append --members ARBDB/*.h `find . -name '*.[h]xx' -type f`
		$(CTAGS) --append ARBDB/*.c `find . -name '*.[c]xx' -type f`

# if the above tag creation does not work -> try tags2:
tags2:
		ctags    -e --c-types=cdt --sort=no `find . \( -name '*.[ch]xx' -o -name "*.[ch]" \) -type f | grep -v -i perl5`
		ctags -a -e --c-types=f-tvx --sort=no `find . \( -name '*.[ch]xx' -o -name "*.[ch]" \) -type f | grep -v -i perl5`

#********************************************************************************

ifndef DEBIAN
links: SOURCE_TOOLS/generate_all_links.stamp
else
links:
	@echo ARB authors do some stuff with symlinks here.  This is not necessary with Debian.
endif

SOURCE_TOOLS/generate_all_links.stamp: SOURCE_TOOLS/generate_all_links.sh
	-sh SOURCE_TOOLS/generate_all_links.sh
	touch SOURCE_TOOLS/generate_all_links.stamp

gde:		GDE/GDE.dummy
agde: 		ARB_GDE/ARB_GDE.dummy
#ps:		$(ARCHS_PROBE_SET:.a=.dummy) $(PROBE_SET)
ps:			PROBE_SET/PROBE_SET.dummy
tools:		TOOLS/TOOLS.dummy
nf77:		NIELS_F77/NIELS_F77.dummy
trs:		TRS/TRS.dummy
convert:	CONVERTALN/CONVERTALN.dummy
readseq:	READSEQ/READSEQ.dummy
aleio:		ALEIO/.dummy

#***************************************************************************************
#			Some user commands
#***************************************************************************************
rtc_patch:
	rtc_patch_area -so LIBLINK/libRTC8M.so

menus:
	$(GMAKE) -C GDEHELP -r "PP=$(PP)" all

tarfile:	rebuild
	util/arb_compress
tarfile_ignore: all
	util/arb_compress
tarale:
	util/arb_compress_emacs

sourcetarfile:
		util/arb_save

export:	tarfile sourcetarfile
	util/arb_export /beta
#	util/arb_export

binlink:
	(cd bin; $(MAKE) all);

all: tests arb libs gde tools readseq convert openwinprogs aleio binlink $(SITE_DEPENDEND_TARGETS)
		@echo -----------------------------------
		@echo 'make all' has been done successful
#	(cd LIBLINK; for i in *.s*; do if test -r $$i; then cp $$i  ../lib; fi; done )

# the following lib is not provided with the source
# you need to install Motif (NOT lesstif) and correct
# MOTIF_LIBPATH

ifndef MOTIF_LIBNAME
MOTIF_LIBNAME=libXm.so.2
endif
ifndef MOTIF_LIBPATH
MOTIF_LIBPATH=Motif/$(MOTIF_LIBNAME)
endif

ifndef DEBIAN
libs:	lib/libARBDB.$(SHARED_LIB_SUFFIX) \
	lib/libARBDBPP.$(SHARED_LIB_SUFFIX) \
	lib/libARBDO.$(SHARED_LIB_SUFFIX) \
	lib/libAW.$(SHARED_LIB_SUFFIX) \
	lib/libAWT.$(SHARED_LIB_SUFFIX) \
	lib/$(MOTIF_LIBNAME)
else
libs:	lib/libARBDB.$(SHARED_LIB_SUFFIX) \
	lib/libARBDBPP.$(SHARED_LIB_SUFFIX) \
	lib/libARBDO.$(SHARED_LIB_SUFFIX) \
	lib/libAW.$(SHARED_LIB_SUFFIX) \
	lib/libAWT.$(SHARED_LIB_SUFFIX)
endif

lib/lib%.$(SHARED_LIB_SUFFIX): LIBLINK/lib%.$(SHARED_LIB_SUFFIX)
	cp $< $@

lib/$(MOTIF_LIBNAME):  $(MOTIF_LIBPATH)
	cp $< $@

bin/arb_%:	DEPOT2/%
	cp $< $@
bin/%:	DEPOT2/%
	cp $< $@


ifdef OPENWINHOME
openwinprogs:	gde	$(DEST_BIN)/arb_gde
else
openwinprogs:
endif

nas:
		(cd lib;$(MAKE) nameserver)

perl:	lib/ARB.pm


lib/ARB.pm:	ARBDB/ad_prot.h ARBDB/ad_t_prot.h PERL2ARB/ARB.xs.default
ifdef PERLBIN
	mkdir -p PERL5/bin
	(cd PERL5/bin;ln -f -s ${PERLBIN}/perl .);
endif
	rm -f lib/perl5
ifdef PERLLIB
	(cd lib;ln -f -s ${PERLLIB}/perl5 .);
else
	(cd lib;ln -f -s ../PERL5/perl5 .);
endif
	rm -f PERL2ARB/ARB.xs
	rm -f PERL2ARB/proto.h
	cat ARBDB/ad_prot.h ARBDB/ad_t_prot.h >PERL2ARB/proto.h
	LD_LIBRARY_PATH=${ARBHOME}/LIBLINK;export LD_LIBRARY_PATH;echo LD_LIBRARY_PATH=$$LD_LIBRARY_PATH;echo calling bin/arb_proto_2_xsub ...;bin/arb_proto_2_xsub PERL2ARB/proto.h PERL2ARB/ARB.xs.default >PERL2ARB/ARB.xs
	PATH=/usr/arb/bin:${PATH};export PATH;cd PERL2ARB;echo calling perl ${MACH}.PL;perl -I ../lib/perl5 ${MACH}.PL;echo -------- calling MakeMaker makefile;make
#	PATH=/usr/arb/bin:${PATH};export PATH;cd PERL2ARB;echo calling perl ${MACH}.PL;perl ${MACH}.PL;echo calling make;make
	echo -------- end of MakeMaker-Makefile
	cp PERL2ARB/blib/arch/auto/ARB/ARB.so lib
	cp PERL2ARB/ARB.pm lib
	echo Make lib/ARB.pm and lib/ARB.so finished.

wc:
	wc `find . -type f \( -name '*.[ch]' -o -name '*.[ch]xx' \) -print`

rmbak:
	find . \( -name '*%' -o -name '*.bak' -o -name 'core' \
		-o -name 'infile' -o -name treefile -o -name outfile \
		-o -name 'gde*_?' -o -name '*~' \) \
	-print -exec rm {} \;
	rm -f -r .test.?.er
	rm -f checkpoint*
	rm -f test.?.er

bclean:		#binary clean
	rm -f bin/arb_*
	cd bin;make all

clean:	rmbak
	rm -f `find . -type f \( -name 'core' -o -name '*.o' -o -name '*.a' ! -type l \) -print`
	rm -f *_COM/GENH/*.h
	rm -f *_COM/GENC/*.c

realclean: clean
	rm -f `find bin -type f -perm -001 -print`
	rm -f AISC/aisc
	rm -f AISC_MKPTPS/aisc_mkpt
	rm -f SOURCE_TOOLS/generate_all_links.stamp

rebuild:
		$(MAKE) realclean
		$(MAKE) all

#*** basic arb libraries
arbbasic: links
		$(MAKE) arbbasic2
arbbasic2: mbin menus com nas ${MAKE_RTC}

arbxtras: tg

arbshared: dball aw dp awt
arbapplications: nt pa ed e4 we pr pg na al nal di ph ds trs

arb: arbbasic arbshared arbapplications help arbxtras
#arb: arbbasic db aw dp awt dbs nt pa ed e4 we pr pg na al nal di db2 ph ds trs help arbxtras

save:	rmbak
	util/arb_save

savedepot: rmbak
	util/arb_save_depot

# DO NOT DELETE
