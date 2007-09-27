
#********************* Start of user defined Section

# get the machine type
# (first call make all once to generate this file)

include config.makefile

# ********************************************************************************
#
# The ARB source code is aware of the following defines
#
# GNU					activates __attribute__ definitions
# HAVE_BOOL				should be true if compiler supports the type 'bool'
# $(MACH)				name of the machine (LINUX,SUN4,SUN5,HP,SGI or DIGITAL; see config.makefile)
# NO_REGEXPR			for machines w/o regular expression library
#
# DEBUG					compiles the DEBUG sections
# NDEBUG				doesnt compile the DEBUG sections
# DEBUG_GRAPHICS 		all X-graphics are flushed immediately (for debugging)
# DEVEL_$(DEVELOPER)	developer-dependent flag (enables you to have private sections in code)
#                       DEVELOPER='ANY' (default setting) will be ignored
#                       configurable in config.makefile
# OPENGL=0/1			whether OPENGL is available
# OPENWINHOME			whether openwin programs shall be compiled (arb_gde, gde)
# ARB_64=0/1        	1=>compile 64 bit version
#
#
#********************* Default set and gcc static enviroments *****************
#
FORCEMASK = umask 002

#---------------------- Some compiler-specific defaults

ifdef ECGS
dflag1 = -g -g3
enumequiv =
havebool = -DHAVE_BOOL
else
dflag1 = -g
enumequiv = -fenum-int-equiv
havebool =
endif

#---------------------- developer specific settings

DEVEL_DEF=-DDEVEL_$(DEVELOPER)

ifeq ($(DEVELOPER),ANY) # default setting (skip all developer specific code)
DEVEL_DEF=
endif

ifeq ($(DEVELOPER),RALFX) # special settings for RALFX
DEVEL_DEF=-DDEVEL_RALF
endif

ifeq ($(DEVELOPER),HARALDX) # special settings for HARALDX
DEVEL_DEF=-DDEVEL_HARALD
endif

ifeq ($(OPENGL),1) # activate OPENGL code
DEVEL_DEF:=$(DEVEL_DEF) -DARB_OPENGL
endif

#----------------------

ifeq ($(DEBUG),1)
ifeq ($(DEBUG_GRAPHICS),1)
		dflags = -DDEBUG -DDEBUG_GRAPHICS
else
		dflags = -DDEBUG
endif

		cflags = -O0 $(dflag1) $(dflags) $(DEVEL_DEF)
#		cflags = -O2 $(dflag1) $(dflags) $(DEVEL_DEF)
#		cflags = -O4 $(dflag1) $(dflags) $(DEVEL_DEF)

		lflags = $(dflag1)
		fflags = $(dflag1) -C
		extended_warnings         = -Wwrite-strings -Wunused -Wno-aggregate-return -Wshadow
		extended_cpp_warnings     = -Wnon-virtual-dtor -Wreorder -Wpointer-arith 
		extended_cpp_3xx_warnings = -Wdisabled-optimization -Wmissing-format-attribute -Wmissing-noreturn # -Wfloat-equal  
		extended_cpp_4xx_warnings =
		POST_COMPILE= 2>&1 | $(ARBHOME)/SOURCE_TOOLS/postcompile.pl
else
ifeq ($(DEBUG),0)
		dflags = -DNDEBUG
		cflags = -O4 $(dflags) $(DEVEL_DEF)
		lflags =
		fflags =
		extended_warnings =
		extended_cpp_warnings =
		POST_COMPILE=
endif
endif

ifeq ($(ARB_64),1)
		cflags := -DARB_64
endif

# ---------------------- compiler version detection

GCC=gcc
GPP=g++ -fmessage-length=0
CPPreal=cpp

# supported compiler versions:

ALLOWED_GCC_295_VERSIONS=2.95.3
ALLOWED_GCC_3xx_VERSIONS=3.2 3.3.1 3.3.3 3.3.4 3.3.5 3.3.6 3.4.0 3.4.2 3.4.3
ALLOWED_GCC_4xx_VERSIONS=4.0.0 4.0.2 4.0.3 4.1.1 4.1.2 4.2.0
ALLOWED_GCC_VERSIONS=$(ALLOWED_GCC_295_VERSIONS) $(ALLOWED_GCC_3xx_VERSIONS) $(ALLOWED_GCC_4xx_VERSIONS)

GCC_VERSION_FOUND=$(shell $(GCC) -dumpversion)
GCC_VERSION_ALLOWED=$(strip $(subst ___,,$(foreach version,$(ALLOWED_GCC_VERSIONS),$(findstring ___$(version)___,___$(GCC_VERSION_FOUND)___))))

#---------------------- depending on gcc version add extra warnings

ifeq ($(DEBUG),1)
USING_GCC_3XX=$(strip $(foreach version,$(ALLOWED_GCC_3xx_VERSIONS),$(findstring $(version),$(GCC_VERSION_ALLOWED))))
USING_GCC_4XX=$(strip $(foreach version,$(ALLOWED_GCC_4xx_VERSIONS),$(findstring $(version),$(GCC_VERSION_ALLOWED))))

ifneq ('$(USING_GCC_3XX)','')
extended_cpp_warnings := $(extended_cpp_warnings) $(extended_cpp_3xx_warnings)
endif
ifneq ('$(USING_GCC_4XX)','')
extended_cpp_warnings := $(extended_cpp_warnings) $(extended_cpp_3xx_warnings) $(extended_cpp_4xx_warnings)
endif
endif

#---------------------- machine/OS specific definitions

ifdef DEBIAN
   XHOME = /usr/X11R6
else
   XHOME = /usr/X11
endif

   PREFIX =
   LIBDIR = /usr/lib

   GMAKE = gmake -r
   CPP = $(GPP) -W -Wall $(enumequiv) -D$(MACH) $(havebool) -pipe#		# C++ Compiler /Linker
   PP = $(CPPreal)
   ACC = $(GCC) -W -Wall $(enumequiv) -D$(MACH) -pipe#				# Ansi C
   CCLIB = $(ACC)#			# Ansi C. for shared libraries
   CCPLIB = $(CPP)#			# Same for c++
   AR = ld -r -o#			# Archive Linker
   ARLIB = ld -r -o#			# The same for shared libs.
   XAR = $(AR)# 			# Linker for archives containing templates
   MAKEDEPEND = $(FORCEMASK);makedepend
   SHARED_LIB_SUFFIX = so#		# shared lib suffix
   F77 = f77

   CTAGS = etags
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
   lflags = $(LDFLAGS) -force_flat_namespace -Wl,-stack_size -Wl,10000000 -Wl,-stack_addr -Wl,c0000000
   DARWIN_SPECIALS = -DNO_REGEXPR  -no-cpp-precomp -DHAVE_BOOL
   CPP = g++2 -D$(MACH) $(DARWIN_SPECIALS)
   ACC = gcc2 -D$(MACH) $(DARWIN_SPECIALS)
   CCLIB = gcc2 -fno-common -D$(MACH) $(DARWIN_SPECIALS)
   CCPLIB = g++2 -fno-common -D$(MACH) $(DARWIN_SPECIALS)
   AR = ld -r -o#                 # Archive Linker
   ARLIB = $(AR)#                 # Archive Linker shared libs.
#  ARLIB = gcc2 -bundle -flat_namespace -undefined suppress -o
   SHARED_LIB_SUFFIX = a#
# .. Just building shared libraries static, i was having problems otherwise

   GMAKE = make -j 3 -r
   SYSLIBS = -lstdc++

   MOTIF_LIBNAME = libXm.3.dylib
   MOTIF_LIBPATH = $(LIBDIR)/$(MOTIF_LIBNAME)
   XINCLUDES = -I/usr/X11R6/include
   XLIBS = -L$(XHOME)/lib -lXm -lXt -lX11 -lXext -lXp  -lc

   PERLBIN = /usr/bin
   PERLLIB = /usr/lib
   CRYPTLIB = -L/usr/lib -lcrypt

endif

#********************* Linux and gcc enviroments *****************
ifdef LINUX

   LINUX_SPECIALS = -DNO_REGEXPR -DGNU
   SITE_DEPENDEND_TARGETS = perl
   CPP := $(CPP) $(LINUX_SPECIALS) $(extended_warnings) $(extended_cpp_warnings)
   ACC := $(ACC) $(LINUX_SPECIALS) $(extended_warnings)
   CCLIB = $(ACC) -fPIC
   CCPLIB = $(CPP) -fPIC#			# Same for c++

   f77_flags = $(fflags) -W -N9 -e
   F77LIB = -lU77

   ARCPPLIB = $(GPP) -Wall -shared $(LINUX_SPECIALS) -o
   ARLIB = $(GCC) -Wall -shared $(LINUX_SPECIALS) -o
   GMAKE = make -j 3 -r
   SYSLIBS = -lm

ifdef DEBIAN
   X11R6=1
else
ifdef REDHAT
   X11R6=1
else
   X11R6=0
endif
endif

# OPENGL specials:

GL=
GL_LIB=
GL_PNGLIBS=
GLLIBS=

ifeq ($(OPENGL),1)
GL:=gl # this is the name of the OPENGL base target
ifdef DEBIAN
GL_LIB:=-lGL -lpthread
else
GL_LIB:=-lGL
endif
GL_PNGLIBS:= -L$(ARBHOME)/GL/glpng -lglpng_arb -lpng
GLLIBS:=-L$(XHOME)/lib -lGLEW -lGLw -lglut $(GL_PNGLIBS)
endif


ifeq ($(X11R6),1)
   XINCLUDES = -I/usr/X11R6/include
   XLIBS = -L/usr/X11R6/lib -lXm -lXpm -lXp -lXt -lXext -lX11 -L$(XHOME)/lib -lc $(GL_LIB)
else
   XINCLUDES = -I/usr/X11/include -I/usr/X11/include/Xm -I$(OPENWINHOME)/include
   XLIBS = -lXm -lXpm -lXp -lXt -lXext -lX11 -L$(XHOME)/lib -lc $(GL_LIB)
endif

   OWLIBS =  -L${OPENWINHOME}/lib -lxview -lolgx -L$(XHOME)/lib -lX11  -lc
   PERLBIN = /usr/bin
   PERLLIB = /usr/lib
   CRYPTLIB = -L/usr/lib -lcrypt

endif

#********************* SUN5  CC enviroments  *****************
#********************* SUN5  ****
ifdef SUN5
   SITE_DEPENDEND_TARGETS = perl

   # gcc on solaris:
   SUN5_ECGS_SPECIALS=-DNO_REGEXPR

   CPP = $(GPP) -W -Wall $(enumequiv) -D$(MACH)_ECGS $(havebool) -pipe $(SUN5_ECGS_SPECIALS)#		# C++ Compiler /Linker
   ACC = $(GCC) -Wall $(enumequiv) -D$(MACH)_ECGS -pipe $(SUN5_ECGS_SPECIALS)#				# Ansi C

   CCLIB = $(ACC) -fPIC
   CCPLIB = $(CPP) -fPIC#			# Same for c++

   ARCPPLIB = $(GPP) -Wall -shared $(SUN5_ECGS_SPECIALS) -o
   ARLIB = $(GCC) -Wall -shared $(SUN5_ECGS_SPECIALS) -o

   XAR = $(AR)# 			# Linker for archives containing templates

   XINCLUDES = -I/usr/X11/include -I/usr/X11/include/Xm -I$(OPENWINHOME)/include

   SYSLIBS = -lsocket -lm -lnsl -lgen -lposix4
   XLIBS =  -L$(OPENWINHOME)/lib -L$(XHOME)/lib -lXm -lXt -lX11

endif

#********************* End of user defined Section *******************

#********************* Main dependences *******************
ifndef ARCPPLIB
	ARCPPLIB = $(ARLIB)
endif

SEP=--------------------------------------------------------------------------------

first_target:
		$(MAKE) checks
		@echo $(SEP)
		@echo 'Main targets:'
		@echo ''
		@echo ' all         - Compile ARB + TOOLs + and copy shared libs + link foreign software'
		@echo '               (That is most likely the target you want)'
		@echo ''
		@echo ' clean       - remove generated files ("SUBDIR/SUBDIR.clean" to clean only SUBDIR)'
		@echo ' rebuild     - clean + all'
		@echo ' relink      - remove all binaries and relink them from objects'
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
		@echo ' depends      - create or update dependencies ("SUBDIR/SUBDIR.depends" to update only SUBDIR)'
		@echo ' proto        - create or update prototypes ("SUBDIR/SUBDIR.proto" to update only SUBDIR)'
		@echo ' tags         - create tags for xemacs'
		@echo ' rmbak        - remove all "*%" and cores'
		@echo ' show         - show available shortcuts (AKA subtargets)'
		@echo ' up           - shortcut for depends+proto+tags'
		@echo ''
		@echo 'Internal maintainance:'
		@echo ''
		@echo ' release     - make tarfile + make save ("release_quick" to skip rebuild)'
		@echo ' tarfile     - make rebuild and create arb version tarfile ("tarfile_quick" to skip rebuild)'
#		@echo ' tarale      - compress emacs and ale lisp files int arb_ale.tar.gz'
		@echo ' save        - save all basic ARB sources into arbsrc_DATE'
#		@echo ' savedepot   - save all extended ARB source (DEPOT2 subdir) into arbdepot_DATE.cpio.gz'
		@echo ' rtc_patch   - create LIBLINK/libRTC8M.so (SOLARIS ONLY)'
		@echo ' source_doc  - create doxygen documentation'
		@echo ''
		@echo $(SEP)
		@echo ''


# auto-generate config.makefile:

CONFIG_MAKEFILE_FOUND=$(wildcard config.makefile)

config.makefile : config.makefile.template
		@echo --------------------------------------------------------------------------------
ifeq ($(strip $(CONFIG_MAKEFILE_FOUND)),)
		@cp $< $@
		@echo '$@:1: has been generated.'
		@echo 'Please edit $@ to configure your system!'
		@echo '(not needed for linux systems - simply type "make all")'
else
		@echo '$<:1: is more recent than'
		@echo '$@:1:'
		@ls -al config.makefile*
		@echo "you may either:"
		@echo "- ignore it and touch $@"
		@echo "- merge difference between $@ and $< (recommended)"
		@echo "- remove $@, run make again and edit the freshly generated $@"
endif
		@echo --------------------------------------------------------------------------------
		@false

# check if everything is configured correctly

check_DEVELOPER:
ifndef DEVELOPER
		@echo 'config.makefile:1: DEVELOPER not defined'
		@false
endif

check_DEBUG:
ifndef dflags
		@echo 'config.makefile:1: DEBUG has to be defined. Valid values are 0 and 1'
		@false
endif

# ---------------------------------------- check gcc version

check_same_GCC_VERSION:
		$(ARBHOME)/SOURCE_TOOLS/check_same_gcc_version.pl $(GCC_VERSION_ALLOWED)

check_GCC_VERSION:
		@echo 'GCC version check:'
		@echo "  - Your version is '$(GCC_VERSION_FOUND)'"
ifeq ('$(GCC_VERSION_ALLOWED)', '')
		@echo '  - This version is not in the list of supported gcc-versions:'
		@$(foreach version,$(ALLOWED_GCC_VERSIONS),echo '    * $(version)';)
		@echo '  - You may either ..'
		@echo '    - add your version to ALLOWED_GCC_VERSIONS in the Makefile and try it out or'
		@echo '    - switch to one of the allowed versions (see arb_README_gcc.txt for installing'
		@echo '      a different version of gcc)'
		@echo ''
		@/bin/false
else
		@echo "  - Supported gcc version '$(GCC_VERSION_ALLOWED)' detected - fine!"
		@echo ''
		$(MAKE) check_same_GCC_VERSION

endif

GCC_WITH_VTABLE_AFTER_CLASS=$(ALLOWED_GCC_295_VERSIONS)
HAVE_GCC_WITH_VTABLE_AFTER_CLASS=$(strip $(foreach version,$(GCC_WITH_VTABLE_AFTER_CLASS),$(findstring $(version),$(GCC_VERSION_ALLOWED))))

# depending on the version of gcc the location of the vtable pointer differs.
ifeq ('$(HAVE_GCC_WITH_VTABLE_AFTER_CLASS)', '')
VTABLE_INFRONTOF_CLASS=1
else
VTABLE_INFRONTOF_CLASS=0
endif

#---------------------- check ARBHOME

# use arb_INSTALL.txt to determine whether ARBHOME points to correct directory
ARB_INSTALL_FOUND=$(wildcard $(ARBHOME)/arb_INSTALL.txt)

check_ARBHOME:
ifeq ($(strip $(ARB_INSTALL_FOUND)),)
		@echo ------------------------------------------------------------
		@echo "ARBHOME is set to '$(ARBHOME)'"
		@echo "The environment variable ARBHOME has to point to the top arb source directory."
		@echo "If you use bash enter:"
		@echo "          export ARBHOME='`pwd`'"
		@echo ------------------------------------------------------------
		@false
endif

ARB_PATH_SET=$(findstring $(ARBHOME)/bin,$(PATH))

check_PATH: check_ARBHOME
ifeq ($(strip $(ARB_PATH_SET)),)
		@echo ------------------------------------------------------------
		@echo "The environment variable PATH has to contain $(ARBHOME)/bin"
		@echo "If you use bash enter:"
		@echo '			export PATH=$$ARBHOME/bin:$$PATH'
		@echo ------------------------------------------------------------
		@false
endif

check_ENVIRONMENT : check_PATH

# ---------------------

checks: check_ENVIRONMENT check_DEBUG check_DEVELOPER check_GCC_VERSION
		@echo Your setup seems to be ok.


# end test section ------------------------------

DIR = $(ARBHOME)

ARBDB_LIB=-lARBDB
ARBDBPP_LIB=-lARBDBPP

LIBS = $(ARBDB_LIB) $(SYSLIBS)
GUI_LIBS = $(LIBS) -lAW -lAWT $(RTC) $(XLIBS)

LIBPATH = -L$(ARBHOME)/LIBLINK

DEST_LIB = lib
DEST_BIN = bin

AINCLUDES = 	-I. -I$(DIR)/INCLUDE $(XINCLUDES)
CPPINCLUDES =	-I. -I$(DIR)/INCLUDE $(XINCLUDES)
MAKEDEPENDFLAGS = -- $(cflags) -I. -Y$(DIR)/INCLUDE --

ifeq ($(VTABLE_INFRONTOF_CLASS),1)
# Some code in ARB depends on the location of the vtable pointer
# (it does a cast from class AP_tree to struct GBT_TREE). In order to
# work around that hack properly, we define FAKE_VTAB_PTR
# if the vtable is located at the beginning of class.
# We are really sorry for that hack.
cflags:=$(cflags) -DFAKE_VTAB_PTR=char
endif

#*****		List of all Directories

#			ALEIO/ALEIO.a \

ARCHS = \
			AISC/dummy.a \
			AISC_MKPTPS/dummy.a \
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
			CONSENSUS_TREE/CONSENSUS_TREE.a \
			CONVERTALN/CONVERTALN.a \
			DBSERVER/DBSERVER.a \
			DIST/DIST.a \
			EDIT/EDIT.a \
			EDIT4/EDIT4.a \
			EISPACK/EISPACK.a \
			GDE/GDE.a \
			GENOM/GENOM.a \
			GENOM_IMPORT/GENOM_IMPORT.a \
			GL/GL.a \
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
			PGT/PGT.a \
			PHYLO/PHYLO.a \
			PRIMER_DESIGN/PRIMER_DESIGN.a \
			PROBE/PROBE.a \
			PROBE_COM/server.a \
			PROBE_DESIGN/PROBE_DESIGN.a \
			PROBE_SERVER/PROBE_SERVER.a \
			PROBE_SET/PROBE_SET.a \
			READSEQ/READSEQ.a \
			RNA3D/RNA3D.a \
			SECEDIT/SECEDIT.a \
			SEER/SEER.a \
			SEQ_QUALITY/SEQ_QUALITY.a \
			SERVERCNTRL/SERVERCNTRL.a \
			SL/SL.a \
			STAT/STAT.a \
			TEST/TEST.a \
			TOOLS/TOOLS.a \
			TREEGEN/TREEGEN.a \
			TRS/TRS.a \
			WETC/WETC.a \
			WINDOW/libAW.a \
			XML/XML.a \

#ARCHS_CLIENTACC = PROBE_COM/client.a
#ARCHS_CLIENTCPP = NAMES_COM/client.a
ARCHS_CLIENT_PROBE = PROBE_COM/client.a
ARCHS_CLIENT_NAMES = NAMES_COM/client.a
#ARCHS_CLIENT = $(ARCHS_CLIENT_NAMES)
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
		$(ARCHS_CLIENT_PROBE) \
		$(SEERLIB) \
		ARB_GDE/ARB_GDE.a \
		AWTC/AWTC.a \
		AWTI/AWTI.a \
		CAT/CAT.a \
		GENOM/GENOM.a \
		GENOM_IMPORT/GENOM_IMPORT.a \
		ISLAND_HOPPING/ISLAND_HOPPING.a \
		MERGE/MERGE.a \
		MULTI_PROBE/MULTI_PROBE.a \
		NTREE/NTREE.a \
		PRIMER_DESIGN/PRIMER_DESIGN.a \
		PROBE_DESIGN/PROBE_DESIGN.a \
		SEQ_QUALITY/SEQ_QUALITY.a \
		SERVERCNTRL/SERVERCNTRL.a \
		SL/AW_NAME/AW_NAME.a \
		SL/HELIX/HELIX.a \
		SL/DB_SCANNER/DB_SCANNER.a \
		SL/FILE_BUFFER/FILE_BUFFER.a \
		STAT/STAT.a \
		XML/XML.a \

$(NTREE): $(ARCHS_NTREE:.a=.dummy) NAMES_COM/server.dummy shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_NTREE) $(GUI_LIBS) || ( \
		echo Link $@ ; \
		echo $(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_NTREE) $(GUI_LIBS) ; \
		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_NTREE) $(GUI_LIBS)  \
		)

#*********************************** arb_rna3d **************************************
RNA3D = bin/arb_rna3d
ARCHS_RNA3D = \
		RNA3D/RNA3D.a \

$(RNA3D): $(ARCHS_RNA3D:.a=.dummy) shared_libs
	@echo $@ currently does not work as standalone application
	false

#***********************************	arb_edit **************************************
EDIT = bin/arb_edit
ARCHS_EDIT = \
		NAMES_COM/client.a \
		SERVERCNTRL/SERVERCNTRL.a \
		EDIT/EDIT.a \
		ARB_GDE/ARB_GDE.a \
		STAT/STAT.a \
		XML/XML.a \
		SL/HELIX/HELIX.a \
		SL/AW_HELIX/AW_HELIX.a \
		SL/AW_NAME/AW_NAME.a \

$(EDIT): $(ARCHS_EDIT:.a=.dummy) shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_EDIT) $(GUI_LIBS) || ( \
		echo Link $@ ; \
		echo $(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_EDIT) -lARBDBPP $(GUI_LIBS) ; \
		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_EDIT) -lARBDBPP $(GUI_LIBS) ; \
		)

#***********************************	arb_edit4 **************************************
EDIT4 = bin/arb_edit4

ARCHS_EDIT4_GENERAL = \
		NAMES_COM/client.a \
		AWTC/AWTC.a \
		EDIT4/EDIT4.a \
		SECEDIT/SECEDIT.a \
		SERVERCNTRL/SERVERCNTRL.a \
		STAT/STAT.a \
		ARB_GDE/ARB_GDE.a \
		ISLAND_HOPPING/ISLAND_HOPPING.a \
		SL/HELIX/HELIX.a \
		SL/AW_HELIX/AW_HELIX.a \
		SL/AW_NAME/AW_NAME.a \
		SL/FILE_BUFFER/FILE_BUFFER.a \
		XML/XML.a \

ifeq ($(OPENGL),0)
ARCHS_EDIT4 = $(ARCHS_EDIT4_GENERAL)
LIBS_EDIT4 =
else
ARCHS_EDIT4 =  \
		RNA3D/RNA3D.a \
		$(ARCHS_EDIT4_GENERAL)
LIBS_EDIT4 = $(GLLIBS)
endif

$(EDIT4): $(ARCHS_EDIT4:.a=.dummy) shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_EDIT4) $(GUI_LIBS) || ( \
		echo Link $@ ; \
		echo $(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_EDIT4) $(GUI_LIBS) $(LIBS_EDIT4) ; \
		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_EDIT4) $(GUI_LIBS) $(LIBS_EDIT4) \
		)

#***********************************	arb_pgt **************************************

PGT = bin/arb_pgt
ARCHS_PGT = \
		PGT/PGT.a \

PGT_SYS_LIBS=$(XLIBS) -ltiff $(LIBS)

$(PGT) : $(ARCHS_PGT:.a=.dummy) shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_PGT) || ( \
		echo Link $@ ; \
		echo $(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PGT) $(PGT_SYS_LIBS); \
		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PGT) $(PGT_SYS_LIBS); \
		)


#***********************************	arb_wetc **************************************
WETC = bin/arb_wetc
ARCHS_WETC = \
		WETC/WETC.a \
		SL/HELIX/HELIX.a \
		XML/XML.a \

$(WETC): $(ARCHS_WETC:.a=.dummy) shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_WETC) $(GUI_LIBS) || ( \
		echo Link $@ ; \
		echo $(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_WETC) $(GUI_LIBS) ; \
		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_WETC) $(GUI_LIBS) ; \
		)

#***********************************	arb_dist **************************************
DIST = bin/arb_dist
ARCHS_DIST = \
		$(ARCHS_CLIENT_PROBE) \
		DIST/DIST.a \
		SERVERCNTRL/SERVERCNTRL.a \
		CONSENSUS_TREE/CONSENSUS_TREE.a \
		EISPACK/EISPACK.a \
		SL/HELIX/HELIX.a \
		XML/XML.a \

$(DIST): $(ARCHS_DIST:.a=.dummy) shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_DIST) $(GUI_LIBS) || ( \
		echo Link $@ ; \
		echo $(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_DIST) $(GUI_LIBS) ; \
		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_DIST) $(GUI_LIBS) ; \
		)

#***********************************	arb_pars **************************************
PARSIMONY = bin/arb_pars
ARCHS_PARSIMONY = \
		NAMES_COM/client.a \
		SERVERCNTRL/SERVERCNTRL.a \
		PARSIMONY/PARSIMONY.a \
		SL/HELIX/HELIX.a \
		SL/AW_NAME/AW_NAME.a \
		XML/XML.a \

$(PARSIMONY): $(ARCHS_PARSIMONY:.a=.dummy) shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_PARSIMONY) $(GUI_LIBS) || ( \
		echo Link $@ ; \
		echo $(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PARSIMONY) $(GUI_LIBS) ; \
		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PARSIMONY) $(GUI_LIBS) ; \
		)

#*********************************** arb_treegen **************************************
TREEGEN = bin/arb_treegen
ARCHS_TREEGEN =	\
		TREEGEN/TREEGEN.a \

$(TREEGEN) :  $(ARCHS_TREEGEN:.a=.dummy)
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_TREEGEN) || ( \
		echo Link $@ ; \
		echo $(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_TREEGEN) ; \
		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_TREEGEN) ; \
		)

#***********************************	arb_naligner **************************************
NALIGNER = bin/arb_naligner
ARCHS_NALIGNER = \
		$(ARCHS_CLIENT_PROBE) \
		NALIGNER/NALIGNER.a \
		SERVERCNTRL/SERVERCNTRL.a \
		SL/HELIX/HELIX.a \

$(NALIGNER): $(ARCHS_NALIGNER:.a=.dummy) shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_NALIGNER) || ( \
		echo Link $@ ; \
		echo $(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_NALIGNER) $(LIBS) ; \
		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_NALIGNER) $(LIBS) \
		)

#***********************************	arb_secedit **************************************
SECEDIT = bin/arb_secedit
ARCHS_SECEDIT = \
		SECEDIT/SECEDIT.a \
		XML/XML.a \

$(SECEDIT):	$(ARCHS_SECEDIT:.a=.dummy) shared_libs
	@echo $@ currently does not work as standalone application
	false


#***********************************	arb_phylo **************************************
PHYLO = bin/arb_phylo
ARCHS_PHYLO = \
		PHYLO/PHYLO.a \
		SL/HELIX/HELIX.a \
		XML/XML.a \

$(PHYLO): $(ARCHS_PHYLO:.a=.dummy) shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_PHYLO) $(GUI_LIBS) || ( \
		echo Link $@ ; \
		echo $(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PHYLO) $(GUI_LIBS) ; \
		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PHYLO) $(GUI_LIBS) ; \
		)

#***************************************************************************************
#					SERVER SECTION
#***************************************************************************************

#***********************************	arb_db_server **************************************
DBSERVER = bin/arb_db_server
ARCHS_DBSERVER = \
		DBSERVER/DBSERVER.a \
		SERVERCNTRL/SERVERCNTRL.a \

$(DBSERVER): $(ARCHS_DBSERVER:.a=.dummy) shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_DBSERVER) $(ARBDB_LIB) || ( \
		echo Link $@ ; \
		echo $(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_DBSERVER) $(ARBDB_LIB) $(SYSLIBS) ; \
		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_DBSERVER) $(ARBDB_LIB) $(SYSLIBS) ; \
		)

#***********************************	arb_pt_server **************************************
PROBE = bin/arb_pt_server
ARCHS_PROBE = \
		PROBE_COM/server.a \
		PROBE/PROBE.a \
		SERVERCNTRL/SERVERCNTRL.a \
		SL/HELIX/HELIX.a \

$(PROBE): $(ARCHS_PROBE:.a=.dummy) shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_PROBE) $(ARBDB_LIB) $(ARCHS_CLIENT_PROBE) || ( \
		echo Link $@ ; \
		echo $(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PROBE) $(ARBDB_LIB) $(ARCHS_CLIENT_PROBE) $(SYSLIBS) ; \
		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PROBE) $(ARBDB_LIB) $(ARCHS_CLIENT_PROBE) $(SYSLIBS) ; \
		)

#***********************************	arb_name_server **************************************
NAMES = bin/arb_name_server
ARCHS_NAMES = \
		NAMES_COM/server.a \
		NAMES/NAMES.a \
		SERVERCNTRL/SERVERCNTRL.a \

$(NAMES): $(ARCHS_NAMES:.a=.dummy) shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_NAMES) $(ARBDB_LIB) $(ARCHS_CLIENT_NAMES) || ( \
		echo Link $@ ; \
		echo $(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_NAMES) $(ARBDB_LIB) $(ARCHS_CLIENT_NAMES) $(SYSLIBS) ; \
		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_NAMES) $(ARBDB_LIB) $(ARCHS_CLIENT_NAMES) $(SYSLIBS) ; \
		)

#***********************************	ors **************************************
ORS_SERVER = tb/ors_server
ARCHS_ORS_SERVER = \
		ORS_COM/server.a \
		ORS_SERVER/ORS_SERVER.a \
		SERVERCNTRL/SERVERCNTRL.a \

$(ORS_SERVER): $(ARCHS_ORS_SERVER:.a=.dummy) shared_libs
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) ORS_SERVER/ORS_SERVER.a SERVERCNTRL/SERVERCNTRL.a ORS_COM/server.a ORS_COM/client.a $(STATIC) $(ARBDB_LIB) $(DYNAMIC) $(SYSLIBS) $(CCPLIBS) $(CRYPTLIB)

ORS_CGI = tb/ors_cgi
ARCHS_ORS_CGI = \
		ORS_COM/server.a \
		ORS_CGI/ORS_CGI.a \
		SERVERCNTRL/SERVERCNTRL.a \

$(ORS_CGI): $(ARCHS_ORS_CGI:.a=.dummy) shared_libs
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) ORS_CGI/ORS_CGI.a SERVERCNTRL/SERVERCNTRL.a ORS_COM/client.a $(STATIC) $(ARBDB_LIB) $(DYNAMIC) $(SYSLIBS) $(CCPLIBS)


EDITDB = tb/editDB
ARCHS_EDITDB = \
		EDITDB/EDITDB.a \
		XML/XML.a \

$(EDITDB): $(ARCHS_EDITDB:.a=.dummy) shared_libs
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(ARCHS_EDITDB) $(ARBDB_LIB) -lAWT $(LIBS)


#***********************************	TEST SECTION  **************************************
AWDEMO = tb/awdemo
ARCHS_AWDEMO = \
		AWDEMO/AWDEMO.a \

$(AWDEMO): $(ARCHS_AWDEMO:.a=.dummy) shared_libs
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(ARCHS_AWDEMO) $(LIBS)

TEST = tb/dbtest
ARCHS_TEST = \
		TEST/TEST.a \
		XML/XML.a \

$(TEST): $(ARCHS_TEST:.a=.dummy) shared_libs
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_TEST)  -lAWT $(LIBS)

ALIV3 = bin/aliv3
ARCHS_ALIV3 = \
		ALIV3/ALIV3.a \
		SL/HELIX/HELIX.a \

$(ALIV3): $(ARCHS_ALIV3:.a=.dummy) shared_libs
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_ALIV3) $(ARBDB_LIB) $(SYSLIBS) $(CCPLIBS)


ACORR = tb/acorr
ARCHS_ACORR = \
		DIST/DIST.a \
		SERVERCNTRL/SERVERCNTRL.a \
		FINDCORRASC/FINDCORRASC.a \
		FINDCORRMATH/FINDCORRMATH.a \
		FINDCORRWIN/FINDCORRWIN.a \
		XML/XML.a \

$(ACORR): $(ARCHS_ACORR:.a=.dummy) shared_libs
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_ACORR) -lAWT $(ARBDBPP_LIB) $(LIBS)


ARBDB_COMPRESS = tb/arbdb_compress
ARCHS_ARBDB_COMPRESS = \
		ARBDB_COMPRESS/ARBDB_COMPRESS.a \

$(ARBDB_COMPRESS): $(ARCHS_ARBDB_COMPRESS:.a=.dummy) shared_libs
	@echo $(SEP) Link $@
	$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_ARBDB_COMPRESS) $(ARBDB_LIB)

#***********************************	OTHER EXECUTABLES   ********************************************


#***********************************	SHARED LIBRARIES SECTION  **************************************

# the following lib is not provided with the source
# you need to install Motif (NOT lesstif) and correct
# MOTIF_LIBPATH

ifndef MOTIF_LIBNAME
MOTIF_LIBNAME=libXm.so.2
endif
ifndef MOTIF_LIBPATH
MOTIF_LIBPATH=Motif/$(MOTIF_LIBNAME)
endif

shared_libs: dball aw awt
		@echo -------------------- Updating shared libraries
		$(MAKE) libs

ifdef DEBIAN
libs:	lib/libARBDB.$(SHARED_LIB_SUFFIX) \
	lib/libARBDBPP.$(SHARED_LIB_SUFFIX) \
	lib/libARBDO.$(SHARED_LIB_SUFFIX) \
	lib/libAW.$(SHARED_LIB_SUFFIX) \
	lib/libAWT.$(SHARED_LIB_SUFFIX)
else
libs:	lib/libARBDB.$(SHARED_LIB_SUFFIX) \
	lib/libARBDBPP.$(SHARED_LIB_SUFFIX) \
	lib/libARBDO.$(SHARED_LIB_SUFFIX) \
	lib/libAW.$(SHARED_LIB_SUFFIX) \
	lib/libAWT.$(SHARED_LIB_SUFFIX) \
#	lib/$(MOTIF_LIBNAME)
endif

lib/lib%.$(SHARED_LIB_SUFFIX): LIBLINK/lib%.$(SHARED_LIB_SUFFIX)
	cp $< $@

lib/$(MOTIF_LIBNAME):  $(MOTIF_LIBPATH)
	cp $< $@

#***************************************************************************************
#			Rekursiv calls to submakefiles
#***************************************************************************************

%.depends:
	@cp -p $(@D)/Makefile $(@D)/Makefile.old # save old Makefile
	@$(MAKE) -C $(@D) -r \
		"AUTODEPENDS=1" \
		"LD_LIBRARY_PATH  = ${LD_LIBRARY_PATH}" \
		"MAKEDEPENDFLAGS = $(MAKEDEPENDFLAGS)" \
		"MAKEDEPEND=$(MAKEDEPEND)" \
		"ARBHOME=$(ARBHOME)" \
		depends;
	@grep "^# DO NOT DELETE" $(@D)/Makefile >/dev/null
	@cat $(@D)/Makefile \
		| SOURCE_TOOLS/fix_depends.pl \
		>$(@D)/Makefile.2
	@mv $(@D)/Makefile.old $(@D)/Makefile # restore old Makefile
	@$(ARBHOME)/SOURCE_TOOLS/mv_if_diff $(@D)/Makefile.2 $(@D)/Makefile # update Makefile if changed

%.proto:
	@$(MAKE) -C $(@D) \
		"AUTODEPENDS=0" \
		proto

%.clean:
	@echo "$(ARBHOME)/$(@D)/Makefile:1: here"
	@$(MAKE) -C $(@D) \
		"AUTODEPENDS=0" \
		"MACH=$(MACH)" \
		"OPENGL=$(OPENGL)" \
		clean

# rule to generate main target (normally a library):
%.dummy:
	@echo $(SEP) Make everything in $(@D)
	@$(MAKE) -C $(@D) -r \
		"AUTODEPENDS=1" \
		"ARBHOME = $(ARBHOME)" \
		"cflags = $(cflags) -D_ARB_$(subst /,_,$(@D))" \
		"lflags = $(lflags)" \
		"CPPINCLUDES = $(CPPINCLUDES)" \
		"AINCLUDES = $(AINCLUDES)" \
		"F77 = $(F77)" \
		"f77_flags = $(f77_flags)" \
		"F77LIB = $(F77LIB)" \
		"CPP = $(CPP)" \
		"ACC = $(ACC)" \
		"CCLIB = $(CCLIB)" \
		"CCPLIB = $(CCPLIB)" \
		"CCPLIBS = $(CCPLIBS)" \
		"AR = $(AR)" \
		"XAR = $(XAR)" \
		"ARLIB = $(ARLIB)" \
		"ARCPPLIB = $(ARCPPLIB)" \
		"LIBPATH = $(LIBPATH)" \
		"SYSLIBS = $(SYSLIBS)" \
		"XHOME = $(XHOME)" \
		"XLIBS = $(XLIBS)" \
		"STATIC = $(STATIC)"\
		"SHARED_LIB_SUFFIX = $(SHARED_LIB_SUFFIX)" \
		"LD_LIBRARY_PATH  = $(LD_LIBRARY_PATH)" \
		"OPENGL  = $(OPENGL)" \
		"POST_COMPILE = $(POST_COMPILE)" \
		"MAIN = $(@F:.dummy=.a)"


# Additional dependencies for subtargets:

comtools: $(ARCHS_MAKEBIN:.a=.dummy)

TOOLS/TOOLS.dummy : shared_libs SERVERCNTRL/SERVERCNTRL.dummy CAT/CAT.dummy PROBE_COM/PROBE_COM.dummy
AWTC/AWTC.dummy : NAMES_COM/NAMES_COM.dummy PROBE_COM/PROBE_COM.dummy

PROBE_COM/PROBE_COM.dummy : comtools
PROBE_COM/server.dummy : comtools
PROBE_COM/client.dummy : comtools

NAMES_COM/NAMES_COM.dummy : comtools
NAMES_COM/server.dummy : comtools
NAMES_COM/client.dummy : comtools

ORS_COM/ORS_COM.dummy : comtools
ORS_COM/server.dummy : comtools
ORS_COM/client.dummy : comtools


#***************************************************************************************
#			Short aliases to make targets
#***************************************************************************************

show:
		@echo $(SEP)
		@echo 'Aliases for often needed targets:'
		@echo ''
		@echo ' executables:'
		@echo ''
		@echo '  nt     arb_ntree'
		@echo '  e4     arb_edit4 (includes secedit)'
		@echo '  di     arb_dist'
		@echo '  ph     arb_phylo'
		@echo '  pa     arb_parsimony'
		@echo '  tg     arb_treegen'
		@echo '  ds     arb_dbserver'
		@echo '  pt     arb_pt_server'
		@echo '  na     arb_name_server'
		@echo ''
		@echo ' libraries:'
		@echo ''
		@echo '  com    communication libraries'
		@echo '  dball  ARB database (all versions: db dbs and db2)'
		@echo '  aw     GUI lib'
		@echo '  awt    GUI toolkit'
		@echo '  awtc   general purpose library'
		@echo '  awti   import/export library'
		@echo '  mp     multi probe library'
		@echo '  ge     genome library'
		@echo '  pd     probe design lib'
		@echo '  prd    primer design lib'
		@echo '  ih     island hopper'
		@echo ''
		@echo ' other targets:'
		@echo ''
		@echo '  help   recompile help files'
		@echo '  test   test arbdb (needs fix)'
		@echo '  demo   GUI demo (needs fix)'
		@echo '  tools  make small tools used by arb'
		@echo ''
		@echo ' optional targets (not build by make all)'
		@echo ''
		@echo '  ps		ARB probe server'
		@echo '  pc		ARB probe client (you need java)'
		@echo ''
		@echo ' foreign targets:'
		@echo ''
		@echo '  gde    GDE'
		@echo '  agde   ARB_GDE'
		@echo ''
		@echo 'for other targets inspect $(ARBHOME)/Makefile'
		@echo ''
		@echo $(SEP)

source_doc:
		doxygen

mbin:	$(ARCHS_MAKEBIN:.a=.dummy)

com:	$(ARCHS_COMMUNICATION:.a=.dummy)

help:   menus xml HELP_SOURCE/dummy.dummy

dball:	db dbs db2 dp
db:		ARBDB/libARBDB.dummy
dbs:	ARBDBS/libARBDB.dummy
db2:	ARBDB2/libARBDB.dummy
dp:		ARBDBPP/libARBDBPP.dummy
aw:		WINDOW/libAW.dummy
awt:	AWT/libAWT.dummy
awtc:	AWTC/AWTC.dummy
awti:	AWTI/AWTI.dummy

ih:	ISLAND_HOPPING/ISLAND_HOPPING.dummy

mp: 	MULTI_PROBE/MULTI_PROBE.dummy
mg:		MERGE/MERGE.dummy
ge: 	GENOM/GENOM.dummy
prd: 	PRIMER_DESIGN/PRIMER_DESIGN.dummy

nt:	menus $(NTREE)
ed:	$(EDIT)

al:	$(ALIGNER)
nal:	$(NALIGNER)
a3:	$(ALIV3)

di:	$(DIST)
ph:	$(PHYLO)
pa:	$(PARSIMONY)
tg:	$(TREEGEN)

3d:	RNA3D/RNA3D.dummy
gl:	GL/GL.dummy
sl:	SL/SL.dummy

ds:	$(DBSERVER)
pt:	$(PROBE)
ps:	PROBE_SERVER/PROBE_SERVER.dummy
pc:	PROBE_WEB/PROBE_WEB.dummy
pst: PROBE_SET/PROBE_SET.dummy
pd:	PROBE_DESIGN/PROBE_DESIGN.dummy
na:	$(NAMES)
os:	$(ORS_SERVER)
oc:	$(ORS_CGI)

ac:	$(ARBDB_COMPRESS) # unused? does not compile

test:	$(TEST)
demo:	$(AWDEMO)

sec: SECEDIT/SECEDIT.dummy

e4:	$(EDIT4)
gi:	GENOM_IMPORT/GENOM_IMPORT.dummy
we:	$(WETC)
eb:	$(EDITDB)

pgt:	$(PGT)
xml:	XML/XML.dummy
templ:	TEMPLATES/TEMPLATES.dummy

#********************************************************************************

up: depends proto tags valgrind_update

#********************************************************************************

depends: $(ARCHS:.a=.depends) \
		HELP_SOURCE/HELP_SOURCE.depends \

proto: AISC_MKPTPS/AISC_MKPTPS.dummy TOOLS/TOOLS.dummy 
		$(MAKE) \
				AISC/AISC.proto \
				ARBDB/ARBDB.proto \
				CONVERTALN/CONVERTALN.proto \
				NTREE/NTREE.proto \
				ORS_CGI/ORS_CGI.proto \
				ORS_SERVER/ORS_SERVER.proto \
				PROBE/PROBE.proto \
				SERVERCNTRL/SERVERCNTRL.proto \
				TRS/TRS.proto \
				AISC_COM/AISC_COM.proto \
				GDE/GDE.proto \

#********************************************************************************

valgrind_update:
		$(MAKE) -C SOURCE_TOOLS valgrind_update

#********************************************************************************

TAGFILE=TAGS
TAGFILE_TMP=TAGS.tmp

tags: tags_$(MACH)
	mv $(TAGFILE_TMP) $(TAGFILE)

tags_LINUX: tags2
tags_SUN5: tags1

tags1:
# first search class definitions
		$(CTAGS) -f $(TAGFILE_TMP)         --language=none "--regex=/^[ \t]*class[ \t]+\([^ \t]+\)/" `find . -name '*.[ch]xx' -type f`
		$(CTAGS) -f $(TAGFILE_TMP) --append --language=none "--regex=/\([^ \t]+\)::/" `find . -name '*.[ch]xx' -type f`
# then append normal tags (headers first)
		$(CTAGS) -f $(TAGFILE_TMP) --append --members ARBDB/*.h `find . -name '*.[h]xx' -type f`
		$(CTAGS) -f $(TAGFILE_TMP) --append ARBDB/*.c `find . -name '*.[c]xx' -type f`

# if the above tag creation does not work -> try tags2:
tags2:
		ctags -f $(TAGFILE_TMP)    -e --c-types=cdt --sort=no `find . \( -name '*.[ch]xx' -o -name "*.[ch]" \) -type f | grep -v -i perl5`
		ctags -f $(TAGFILE_TMP) -a -e --c-types=f-tvx --sort=no `find . \( -name '*.[ch]xx' -o -name "*.[ch]" \) -type f | grep -v -i perl5`

#********************************************************************************

links: SOURCE_TOOLS/generate_all_links.stamp

SOURCE_TOOLS/generate_all_links.stamp: SOURCE_TOOLS/generate_all_links.sh
	SOURCE_TOOLS/generate_all_links.sh
	touch SOURCE_TOOLS/generate_all_links.stamp

gde:		GDE/GDE.dummy
GDE:		gde
agde: 		ARB_GDE/ARB_GDE.dummy
tools:		TOOLS/TOOLS.dummy
nf77:		NIELS_F77/NIELS_F77.dummy
trs:		TRS/TRS.dummy
convert:	CONVERTALN/CONVERTALN.dummy
readseq:	READSEQ/READSEQ.dummy

#***************************************************************************************
#			Some user commands
#***************************************************************************************
rtc_patch:
	rtc_patch_area -so LIBLINK/libRTC8M.so

menus: binlink
	@echo $(SEP) Make everything in GDEHELP
	$(MAKE) -C GDEHELP -r "PP=$(PP)" all

ifeq ($(DEBUG),1)
BIN_TARGET=develall
else
BIN_TARGET=all
endif


binlink:
	$(MAKE) -C bin $(BIN_TARGET)

bin/arb_%:	DEPOT2/%
	cp $< $@
bin/%:	DEPOT2/%
	cp $< $@


ifdef OPENWINHOME
openwinprogs:	gde	$(DEST_BIN)/arb_gde
else
openwinprogs:
endif

preplib:
		(cd lib;$(MAKE) all)

# ---------------------------------------- perl

perl: tools
	@echo $(SEP) Make everything in PERL2ARB
	@$(MAKE) -C PERL2ARB -r -f Makefile.main \
		"AUTODEPENDS=1" \
		"MACH=$(MACH)" \
		"dflags=$(dflags)" \
		"ARBHOME=$(ARBHOME)" \
		"DEBUG=$(DEBUG)" \
		"MAKEDEPEND=$(MAKEDEPEND)" \
		"MAKEDEPENDFLAGS=$(MAKEDEPENDFLAGS)" \
		all

perl_clean:
	@$(MAKE) -C PERL2ARB -r -f Makefile.main \
		"AUTODEPENDS=0" \
		"MACH=$(MACH)" \
		clean

# ----------------------------------------

wc:
	wc `find . -type f \( -name '*.[ch]' -o -name '*.[ch]xx' \) -print`

# ---------------------------------------- cleaning

rmbak:
	find . \( -name '*%' -o -name '*.bak' -o -name 'core' \
		-o -name 'infile' -o -name treefile -o -name outfile \
		-o -name 'gde*_?' -o -name '*~' \) \
	-print -exec rm {} \;

bclean:
	@echo Cleaning bin directory
	find bin -type l -exec rm {} \;
	find bin -type f \! \( -name ".cvsignore" -o -name "Makefile" -o -path "bin/CVS*" \) -exec rm {} \;
	cd bin;make all

libclean:
	rm -f `find . -type f \( -name '*.a' ! -type l \) -print`

clean: $(ARCHS:.a=.clean) \
		GDEHELP/GDEHELP.clean \
		HELP_SOURCE/HELP_SOURCE.clean \
		SOURCE_TOOLS/SOURCE_TOOLS.clean \
		bin/bin.clean \
		perl_clean
	rm -f *.last_gcc

# -----------------------------------

rebuild: clean
	$(MAKE) all

relink: bclean libclean
	$(MAKE) all

save: sourcetarfile 

savedepot: rmbak
	util/arb_save_depot

tarfile: rebuild
	util/arb_compress

tarfile_quick: all
	util/arb_compress

sourcetarfile: rmbak
	util/arb_save

save2: rmbak
		util/arb_save ignore

release:
	@echo Making release
	@echo PATH=$(PATH)
	@echo ARBHOME=$(ARBHOME)
	-rm arb.tgz arbsrc.tgz
	$(MAKE) tarfile sourcetarfile

release_quick:
	-rm arb.tgz arbsrc.tgz
	$(MAKE) tarfile_quick sourcetarfile

# --------------------------------------------------------------------------------

# basic arb libraries
arbbasic: links preplib
		$(MAKE) arbbasic2

arbbasic2: templ mbin com ${MAKE_RTC} sl $(GL)

# shared arb libraries
arbshared: dball aw dp awt

# needed arb applications
arbapplications: nt pa ed e4 we pt na al nal di ph ds trs pgt

# optionally things (no real harm for ARB if any of them fails):
arbxtras: tg ps pc pst a3

tryxtras:
		@echo $(SEP)
		@( $(MAKE) arbxtras || ( \
				echo $(SEP) ;\
				echo "If make process seems to abort here, one of the optional tools failed to build." ;\
				echo "ARB will work nevertheless!" ) )

arb: arbbasic arbshared arbapplications help

all: checks arb libs convert tools gde readseq openwinprogs binlink $(SITE_DEPENDEND_TARGETS)
		-$(MAKE) tryxtras
		@echo $(SEP)
		@echo "made 'all' with success."
		@echo "to start arb enter 'arb'"

# DO NOT DELETE
