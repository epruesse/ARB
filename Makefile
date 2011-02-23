# =============================================================== #
#                                                                 #
#   File      : Makefile                                          #
#                                                                 #
#   Institute of Microbiology (Technical University Munich)       #
#   http://www.arb-home.de/                                       #
#                                                                 #
# =============================================================== #

# -----------------------------------------------------
# The ARB Makefile is aware of the following defines:
#
# BUILDHOST_64=0/1      1=>compile on 64 bit platform (defaults to ARB_64)
# DEVELOPER=name	special compilation (values: ANY,RELEASE,your name)
# OPENGL=0/1            whether OPENGL is available
#
# -----------------------------------------------------
# ARB Makefile and ARB source code are aware of the following defines:
#
# $(MACH)               name of the machine (LINUX or DARWIN; see config.makefile)
# DEBUG                 compiles the DEBUG sections
# DEBUG_GRAPHICS        all X-graphics are flushed immediately (for debugging)
# ARB_64=0/1            1=>compile 64 bit version
# UNIT_TESTS=0/1        1=>compile in unit tests and call them after build
# COVERAGE=0/1/2        compile in gcov support (useful together with UNIT_TESTS=1)
#                       0=no, 1+2=compile in, 1=show
#
# -----------------------------------------------------
# The ARB source code is aware of the following defines:
#
# NDEBUG                doesn't compile the DEBUG sections
# DEVEL_$(DEVELOPER)    developer-dependent flag (enables you to have private sections in code)
#                       DEVELOPER='ANY' (default setting) will be ignored
#                       configurable in config.makefile
#
# -----------------------------------------------------
# Read configuration 
include config.makefile

ifeq ($(LD_LIBRARY_PATH),'')
LD_LIBRARY_PATH:=${ARBHOME}/lib
endif

FORCEMASK = umask 002
NODIR=--no-print-directory

# ---------------------- [unconditionally used options]

GCC:=gcc
GPP:=g++ 
CPPreal:=cpp

# ---------------------- compiler version detection

# supported compiler versions:

ALLOWED_GCC_3xx_VERSIONS=3.2 3.3.1 3.3.3 3.3.4 3.3.5 3.3.6 3.4.0 3.4.2 3.4.3
ALLOWED_GCC_4xx_VERSIONS=\
	4.0.0 4.0.1 4.0.2 4.0.3 \
	4.1.1 4.1.2 4.1.3 \
	4.2.0 4.2.1 4.2.3 4.2.4 \
	4.3 4.3.1 4.3.2 4.3.3 4.3.4 \
	4.4 4.4.1 4.4.3 4.4.5

ALLOWED_GCC_VERSIONS=$(ALLOWED_GCC_3xx_VERSIONS) $(ALLOWED_GCC_4xx_VERSIONS)

GCC_VERSION_FOUND=$(shell $(GCC) -dumpversion)
GCC_VERSION_ALLOWED=$(strip $(subst ___,,$(foreach version,$(ALLOWED_GCC_VERSIONS),$(findstring ___$(version)___,___$(GCC_VERSION_FOUND)___))))

USING_GCC_3XX=$(strip $(foreach version,$(ALLOWED_GCC_3xx_VERSIONS),$(findstring $(version),$(GCC_VERSION_ALLOWED))))
USING_GCC_4XX=$(strip $(foreach version,$(ALLOWED_GCC_4xx_VERSIONS),$(findstring $(version),$(GCC_VERSION_ALLOWED))))

#---------------------- define special directories for non standard builds

ifdef DARWIN
OSX_SDK:=/Developer/SDKs/MacOSX10.5.sdk
OSX_FW:=/System/Library/Frameworks
OSX_FW_OPENGL:=$(OSX_FW)/OpenGL.framework/Versions/A/Libraries
OSX_FW_IMAGEIO:=$(OSX_FW)/ApplicationServices.framework/Versions/A/Frameworks/ImageIO.framework/Versions/A/Resources
endif

#----------------------

ifdef DARWIN
	LINK_STATIC=1# link static
else
	LINK_STATIC=0# link dynamically
endif

shared_cflags :=# flags for shared lib compilation
lflags :=# linker flags
extended_warnings :=# warning flags for C and C++-compiler
extended_cpp_warnings :=# warning flags for C++-compiler only


ifeq ($(DEBUG),0)
	dflags := -DNDEBUG# defines
	cflags := -O4# compiler flags (C and C++)
 ifndef DARWIN
	lflags += -O99 --strip-all# linker flags
 endif
endif

ifeq ($(DEBUG),1)
	dflags := -DDEBUG
	cflags := -O0 -g -g3 -ggdb -ggdb3
#	cflags := -O2 -g -g3 -ggdb -ggdb3 # use this for callgrind (force inlining)
 ifndef DARWIN
	lflags += -g
 endif

# control how much you get spammed
	POST_COMPILE := 2>&1 | $(ARBHOME)/SOURCE_TOOLS/postcompile.pl
#	POST_COMPILE := 2>&1 | $(ARBHOME)/SOURCE_TOOLS/postcompile.pl --no-warnings
#	POST_COMPILE := 2>&1 | $(ARBHOME)/SOURCE_TOOLS/postcompile.pl --only-first-error
#	POST_COMPILE := 2>&1 | $(ARBHOME)/SOURCE_TOOLS/postcompile.pl --no-warnings --only-first-error

# Enable several warnings
	extended_warnings     := -Wwrite-strings -Wunused -Wno-aggregate-return -Wshadow
	extended_cpp_warnings := -Wnon-virtual-dtor -Wreorder -Wpointer-arith 
 ifneq ('$(USING_GCC_3XX)','')
	extended_cpp_warnings += -Wdisabled-optimization -Wmissing-format-attribute
	extended_cpp_warnings += -Wmissing-noreturn # -Wfloat-equal 
 endif
 ifneq ('$(USING_GCC_4XX)','')
#	extended_cpp_warnings += -Wwhatever
 endif

 ifeq ($(DEBUG_GRAPHICS),1)
	dflags += -DDEBUG_GRAPHICS
 endif
endif

#---------------------- developer 

ifneq ($(DEVELOPER),ANY) # ANY=default setting (skip all developer specific code)
ifdef dflags
	dflags += -DDEVEL_$(DEVELOPER)# activate developer/release specific code
endif
endif

#---------------------- 32 or 64 bit

ifndef ARB_64
	ARB_64=0#default to 32bit
endif
ifndef BUILDHOST_64
	BUILDHOST_64:=$(ARB_64)# assume build host is same as version (see config.makefile)
endif

cross_cflags:=
cross_lflags:=

ifeq ($(ARB_64),1)
	dflags += -DARB_64 #-fPIC 
	lflags +=
	shared_cflags += -fPIC

	ifeq ($(BUILDHOST_64),1)
#		build 64-bit ARB version on 64-bit host
		CROSS_LIB:=# empty = autodetect below
		ifdef DARWIN
			cross_cflags += -arch x86_64
			cross_lflags += -arch x86_64
		endif
	else
#		build 64-bit ARB version on 32-bit host
		CROSS_LIB:=/lib64
		cross_cflags += -m64
		cross_lflags += -m64 -m elf_x86_64
	endif
else
	ifeq ($(BUILDHOST_64),1)
#		build 32-bit ARB version on 64-bit host
		CROSS_LIB:=# empty = autodetect below
		cross_cflags += -m32
		cross_lflags += -m32 -m elf_i386 
	else
#		build 32-bit ARB version on 32-bit host
		CROSS_LIB:=/lib
	endif
endif

cflags += $(cross_cflags)
lflags += $(cross_lflags)

ifeq ('$(CROSS_LIB)','')
# autodetect libdir
	ifeq ($(ARB_64),1)
		CROSS_LIB:=`(test -d /lib64 && echo lib64) || echo lib`
	else
		CROSS_LIB:=`(test -d /lib32 && echo lib32) || echo lib`
	endif
endif

#---------------------- unit tests

ifndef UNIT_TESTS
	UNIT_TESTS=0#default is "no tests"
endif
ifeq ($(UNIT_TESTS),1)
	dflags += -DUNIT_TESTS
	UNIT_TESTER_LIB=UNIT_TESTER/UNIT_TESTER.a
else
	UNIT_TESTER_LIB=
endif

#---------------------- use gcov

ifndef COVERAGE
	COVERAGE=0#default is "no"
endif
ifneq ($(COVERAGE),0)
	GCOVFLAGS=-ftest-coverage -fprofile-arcs
	cflags += $(GCOVFLAGS)
	EXECLIBS=-lgcov
else
	GCOVFLAGS=
	EXECLIBS=
endif

#---------------------- other flags

dflags += -D$(MACH) # define machine

ifdef DARWIN
	cflags += -no-cpp-precomp 
	shared_cflags += -fno-common
else
	dflags +=  $(shell getconf LFS_CFLAGS)
endif

cflags += -pipe
cflags += -fmessage-length=0# don't wrap compiler output
cflags += -funit-at-a-time
cflags += -fPIC

#---------------------- X11 location

XHOME:=/usr/X11R6
XINCLUDES:=-I$(XHOME)/include

ifdef DARWIN
	XINCLUDES := -I/sw/include -I$(OSX_SDK)/usr/X11/include -I$(OSX_SDK)/usr/include/krb5 -I/usr/OpenMotif/include #Snow Leopard couldn't find OpenMotif
endif

ifdef DARWIN
	XLIBS := -L/usr/OpenMotif/lib -lXm -L$(XHOME)/lib -lpng -lXt -lX11 -lXext -lXp -lc -lXmu -lXi
	XLIBS += -lGLU -lGL -Wl,-dylib_file,$(OSX_FW_OPENGL)/libGL.dylib:$(OSX_FW_OPENGL)/libGL.dylib
else
	XLIBS:=-L$(XHOME)/$(CROSS_LIB) -lXm -lXpm -lXp -lXt -lXext -lX11
endif

#---------------------- open GL

ifeq ($(OPENGL),1) 
	cflags += -DARB_OPENGL # activate OPENGL code
	GL     := gl # this is the name of the OPENGL base target
	GL_LIB_SYS := -lGL -lGLU
	GL_LIB_ARB := -L$(ARBHOME)/GL/glAW -lglAW

 ifdef DEBIAN
	GL_LIB_SYS += -lpthread
 endif

GL_PNGLIBS_ARB := -L$(ARBHOME)/GL/glpng -lglpng_arb
GL_PNGLIBS_SYS := -lpng

 ifdef DARWIN
	GLEWLIB := -L/usr/lib -lGLEW -L$(OSX_SDK)/usr/X11/lib -lGLw
	GLUTLIB := -L$(XHOME)/lib -lglut
 else 
	GLEWLIB := -lGLEW -lGLw
	GLUTLIB := -lglut
 endif

GL_LIBS_SYS := $(GL_LIB_SYS) $(GL_PNGLIBS_SYS) $(GLEWLIB) $(GLUTLIB)
GL_LIBS_ARB := $(GL_LIB_ARB) $(GL_PNGLIBS_ARB)

RNA3D_LIB := RNA3D/RNA3D.a

else
# OPENGL=0

GL_LIBS_ARB:=# no opengl -> no libs
GL_LIBS_SYS:=# no opengl -> no libs
GL:=# don't build ARB openGL libs

RNA3D_LIB :=

endif



GL_LIBS:=$(GL_LIBS_ARB) $(GL_LIBS_SYS)

#---------------------- tiff lib:

ifdef DARWIN
	TIFFLIBS := -L/usr/local/lib -ltiff -L$(OSX_FW_IMAGEIO) -lTIFF  
else
	TIFFLIBS := -ltiff
endif

#---------------------- basic libs:

SYSLIBS:=

ifdef DARWIN
	SYSLIBS += -lstdc++
else
	SYSLIBS += -lm
endif

#---------------------- include symbols?

ifeq ($(TRACESYM),1)
	ifdef DARWIN
		cdynamic =
		ldynamic =
	else
		cdynamic = -rdynamic
		ldynamic = --export-dynamic 
	endif
endif

# ------------------------------------------------------------------------- 
#	Don't put any machine/version/etc conditionals below!
#	(instead define variables above)
# -------------------------------------------------------------------------

cflags += -W -Wall $(dflags) $(extended_warnings) $(cdynamic)

cppflags := $(extended_cpp_warnings)

ifeq ($(DEVELOPER),RALF)
HAVE_GNUPP0X=`SOURCE_TOOLS/requireVersion.pl 4.3 $(GCC_VERSION_FOUND)`
ifeq ($(HAVE_GNUPP0X),1)
# ensure compatibility with upcoming C++ standard
cppflags += -std=gnu++0x
endif
endif

# compiler settings:

ACC := $(GCC)# compile C
CPP := $(GPP) $(cppflags)# compile C++
ACCLIB := $(ACC) $(shared_cflags)# compile C (shared libs)
CPPLIB := $(CPP) $(shared_cflags)# compile C++ (shared libs)

PP := $(CPPreal)# preprocessor

lflags += $(ldynamic)

LINK_STATIC_LIB := ld $(lflags) -r -o# link static lib
LINK_EXECUTABLE := $(GPP) $(lflags) $(cdynamic) -o# link executable (c++)

ifeq ($(LINK_STATIC),1)
SHARED_LIB_SUFFIX = a# static lib suffix
LINK_SHARED_LIB := $(LINK_STATIC_LIB)
else
SHARED_LIB_SUFFIX = so# shared lib suffix
LINK_SHARED_LIB := $(GPP) $(lflags) -shared $(GCOVFLAGS) -o# link shared lib
endif

# other used tools

ifdef DARWIN
	XMKMF := /usr/X11/bin/xmkmf
else
	XMKMF := /usr/bin/X11/xmkmf
endif
MAKEDEPEND_PLAIN = makedepend

MAKEDEPEND = $(FORCEMASK);$(MAKEDEPEND_PLAIN)

SEP:=--------------------------------------------------------------------------------

# delete variables unused below

lflags:=

# ------------------------- 
#     Main arb targets:     
# ------------------------- 

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
ifeq ($(UNIT_TESTS),1)
		@echo ' unit_tests   - only run tests'
endif
		@echo ' modified     - rebuild files modified in svn checkout (does touch)'
		@echo ' touch        - touch files modified in svn checkout'
		@echo ''
		@echo 'Internal maintenance:'
		@echo ''
		@echo ' rel_minor   - build a release (increases minor version number)'
		@echo ' rel_major   - build a release (increases MAJOR version number)'
		@echo ' tarfile     - make rebuild and create arb version tarfile ("tarfile_quick" to skip rebuild)'
		@echo ' save        - save all basic ARB sources into arbsrc_DATE ("testsave" to check filelist)'
		@echo ' patch       - save svn diff to patchfile'
		@echo ' source_doc  - create doxygen documentation'
		@echo ' relocated   - rebuild partly (use when you have relocated ARBHOME)'
		@echo ' check_res   - check ressource usage'
		@echo ''
		@echo $(SEP)
		@echo ''


# auto-generate config.makefile:

CONFIG_MAKEFILE_FOUND=$(wildcard config.makefile)

config.makefile : config.makefile.template
		@echo --------------------------------------------------------------------------------
ifeq ($(strip $(CONFIG_MAKEFILE_FOUND)),)
		@cp $< $@
		@echo '$(ARBHOME)/$@:1: has been generated.'
		@echo 'Please edit $@ to configure your system!'
		@echo --------------------------------------------------------------------------------
		@false
else
		@echo '$(ARBHOME)/$<:1: is more recent than'
		@echo '$(ARBHOME)/$@:1:'
		@ls -al config.makefile*
		@echo --------------------------------------------------------------------------------
		@echo "Updating $@ (if this fails, check manually)"
		SOURCE_TOOLS/update_config_makefile.pl
		@echo "Sucessfully updated $@"
		@echo --------------------------------------------------------------------------------
		@ls -al config.makefile*
		@echo --------------------------------------------------------------------------------
		@echo "Diff to your old config.makefile:"
		@echo --------------------------------------------------------------------------------
		-diff $@.bak $@
		@echo --------------------------------------------------------------------------------
endif

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

check_ARB_64:
ifndef ARB_64
		@echo 'config.makefile:1: ARB_64 has to be defined. Valid values are 0 and 1'
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
		@false
else
		@echo "  - Supported gcc version '$(GCC_VERSION_ALLOWED)' detected - fine!"
		@echo ''
		$(MAKE) check_same_GCC_VERSION

endif

GCC_WITH_VTABLE_AFTER_CLASS=#occurred only with no longer supported $(ALLOWED_GCC_295_VERSIONS)
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

check_TOOLS:
	@util/arb_check_build_env.pl \
		"$(ACC)" \
		"$(CPP)" \
		"$(GPP)" \
		"$(PP)" \
		"$(ACCLIB)" \
		"$(CPPLIB)" \
		"$(XMKMF)" \
		"$(MAKEDEPEND_PLAIN)" \
		"$(LINK_SHARED_LIB)" \
		"$(LINK_SHARED_LIB)" \


check_ENVIRONMENT : check_PATH check_TOOLS

check_tabs: check_setup
ifeq ($(DEBUG),1)
	@SOURCE_TOOLS/tabBrake.pl
endif

force_tab_check:
	@touch -t 198001010000 SOURCE_TOOLS/tabBrake.stamp
	@$(MAKE) check_tabs


# ---------------------

check_setup: check_ENVIRONMENT check_DEBUG check_ARB_64 check_DEVELOPER check_GCC_VERSION 
		@echo Your setup seems to be ok.

checks: check_setup check_tabs


# end test section ------------------------------

ARBDB_LIB=-lARBDB

LIBS = $(ARBDB_LIB) $(SYSLIBS)
GUI_LIBS = $(LIBS) -lWINDOW -lAWT $(XLIBS)

LIBPATH = -L$(ARBHOME)/LIBLINK

DEST_LIB = lib
DEST_BIN = bin

AINCLUDES := -I. -I$(ARBHOME)/INCLUDE $(XINCLUDES)
CPPINCLUDES := -I. -I$(ARBHOME)/INCLUDE $(XINCLUDES)
MAKEDEPENDFLAGS := -- $(cflags) -- -DARB_OPENGL -DUNIT_TESTS=1 -D__cplusplus -I. -Y$(ARBHOME)/INCLUDE

ifeq ($(VTABLE_INFRONTOF_CLASS),1)
# Some code in ARB depends on the location of the vtable pointer
# (it does a cast from class AP_tree to struct GBT_TREE). In order to
# work around that hack properly, we define FAKE_VTAB_PTR
# if the vtable is located at the beginning of class.
# We are really sorry for that hack.
cflags:=$(cflags) -DFAKE_VTAB_PTR=char
endif

# ------------------------------- 
#     old PTSERVER or PTPAN?

ifeq ($(PTPAN),1)
# PTPAN only libs
ARCHS_PT_SERVER = \
	ptpan/PROBE.a
else
ifeq ($(PTPAN),2)
# special mode to compile both servers (developers only!)
ARCHS_PT_SERVER = \
	ptpan/PROBE.a \
	PROBE/PROBE.a
ARCHS_PT_SERVER_LINK = PROBE/PROBE.a # default to old ptserver
else
# PT-server only libs
ARCHS_PT_SERVER = \
	PROBE/PROBE.a
endif
endif

ifndef ARCHS_PT_SERVER_LINK
ARCHS_PT_SERVER_LINK = $(ARCHS_PT_SERVER)
endif

# ------------------------------- 
#     List of all Directories

ARCHS = \
			$(ARCHS_PT_SERVER) \
			AISC/dummy.a \
			AISC_MKPTPS/dummy.a \
			ALIV3/ALIV3.a \
			ARBDB/libARBDB.a \
			ARB_GDE/ARB_GDE.a \
			AWT/libAWT.a \
			AWTC/AWTC.a \
			AWTI/AWTI.a \
			CONSENSUS_TREE/CONSENSUS_TREE.a \
			CONVERTALN/CONVERTALN.a \
			DBSERVER/DBSERVER.a \
			DIST/DIST.a \
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
			PARSIMONY/PARSIMONY.a \
			PGT/PGT.a \
			PHYLO/PHYLO.a \
			PRIMER_DESIGN/PRIMER_DESIGN.a \
			PROBE_COM/server.a \
			PROBE_DESIGN/PROBE_DESIGN.a \
			PROBE_SET/PROBE_SET.a \
			READSEQ/READSEQ.a \
			RNA3D/RNA3D.a \
			SECEDIT/SECEDIT.a \
			SEQ_QUALITY/SEQ_QUALITY.a \
			SERVERCNTRL/SERVERCNTRL.a \
			SL/SL.a \
			STAT/STAT.a \
			TOOLS/TOOLS.a \
			TREEGEN/TREEGEN.a \
			UNIT_TESTER/UNIT_TESTER.a \
			WETC/WETC.a \
			WINDOW/libWINDOW.a \
			XML/XML.a \

# ----------------------- 
#     library packets     

ARCHS_CLIENT_PROBE = PROBE_COM/client.a
ARCHS_CLIENT_NAMES = NAMES_COM/client.a
ARCHS_MAKEBIN = AISC_MKPTPS/dummy.a AISC/dummy.a

ARCHS_COMMUNICATION =	NAMES_COM/server.a \
			PROBE_COM/server.a

# communication libs need aisc and aisc_mkpts:
$(ARCHS_COMMUNICATION:.a=.dummy) : $(ARCHS_MAKEBIN:.a=.dummy)

ARCHS_SEQUENCE = \
		SL/SEQUENCE/SEQUENCE.a \
		SL/ALIVIEW/ALIVIEW.a \
		SL/PRONUC/PRONUC.a \

ARCHS_TREE = \
		$(ARCHS_SEQUENCE) \
		SL/FILTER/FILTER.a \
		SL/ARB_TREE/ARB_TREE.a \

# parsimony tree (used by NTREE, PARSIMONY, STAT(->EDIT4), DIST(obsolete!))
ARCHS_AP_TREE = \
		$(ARCHS_TREE) \
		SL/AP_TREE/AP_TREE.a \

#***************************************************************************************
#		Individual Programs Section
#***************************************************************************************

#***********************************	arb_ntree **************************************
NTREE = bin/arb_ntree
ARCHS_NTREE = \
		NTREE/NTREE.a \
		$(ARCHS_CLIENT_PROBE) \
		$(ARCHS_AP_TREE) \
		ARB_GDE/ARB_GDE.a \
		AWTC/AWTC.a \
		AWTI/AWTI.a \
		GENOM/GENOM.a \
		GENOM_IMPORT/GENOM_IMPORT.a \
		MERGE/MERGE.a \
		MULTI_PROBE/MULTI_PROBE.a \
		PRIMER_DESIGN/PRIMER_DESIGN.a \
		PROBE_DESIGN/PROBE_DESIGN.a \
		SEQ_QUALITY/SEQ_QUALITY.a \
		SERVERCNTRL/SERVERCNTRL.a \
		SL/AW_NAME/AW_NAME.a \
		SL/DB_SCANNER/DB_SCANNER.a \
		SL/SEQIO/SEQIO.a \
		SL/FILE_BUFFER/FILE_BUFFER.a \
		SL/GUI_ALIVIEW/GUI_ALIVIEW.a \
		SL/HELIX/HELIX.a \
		SL/REGEXPR/REGEXPR.a \
		SL/TRANSLATE/TRANSLATE.a \
		SL/TREEDISP/TREEDISP.a \
		SL/TREE_READ/TREE_READ.a \
		SL/TREE_WRITE/TREE_WRITE.a \
		STAT/STAT.a \
		XML/XML.a \

$(NTREE): $(ARCHS_NTREE:.a=.dummy) NAMES_COM/server.dummy shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_NTREE) $(GUI_LIBS) || ( \
		echo Link $@ ; \
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_NTREE) $(GUI_LIBS) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_NTREE) $(GUI_LIBS) $(EXECLIBS)  \
		)

#*********************************** arb_rna3d **************************************
RNA3D = bin/arb_rna3d
ARCHS_RNA3D = \
		RNA3D/RNA3D.a \

$(RNA3D): $(ARCHS_RNA3D:.a=.dummy) shared_libs
	@echo $@ currently does not work as standalone application
	false

#***********************************	arb_edit4 **************************************
EDIT4 = bin/arb_edit4

ARCHS_EDIT4 := \
		EDIT4/EDIT4.a \
		$(ARCHS_AP_TREE) \
		ARB_GDE/ARB_GDE.a \
		AWTC/AWTC.a \
		ISLAND_HOPPING/ISLAND_HOPPING.a \
		$(ARCHS_CLIENT_NAMES) \
		SECEDIT/SECEDIT.a \
		SERVERCNTRL/SERVERCNTRL.a \
		SL/AW_HELIX/AW_HELIX.a \
		SL/AW_NAME/AW_NAME.a \
		SL/FAST_ALIGNER/FAST_ALIGNER.a \
		SL/FILE_BUFFER/FILE_BUFFER.a \
		SL/GUI_ALIVIEW/GUI_ALIVIEW.a \
		SL/HELIX/HELIX.a \
		SL/TRANSLATE/TRANSLATE.a \
		STAT/STAT.a \
		XML/XML.a \

ifeq ($(OPENGL),1)
ARCHS_EDIT4 += RNA3D/RNA3D.a
endif
LIBS_EDIT4 := $(GL_LIBS)

$(EDIT4): $(ARCHS_EDIT4:.a=.dummy) shared_libs $(GL)
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_EDIT4) $(GUI_LIBS) || ( \
		echo Link $@ ; \
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_EDIT4) $(GUI_LIBS) $(LIBS_EDIT4) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_EDIT4) $(GUI_LIBS) $(LIBS_EDIT4) $(EXECLIBS) \
		)

#***********************************	arb_pgt **************************************

PGT = bin/arb_pgt
ARCHS_PGT = \
		PGT/PGT.a \

PGT_SYS_LIBS=$(XLIBS) $(TIFFLIBS) $(LIBS)

$(PGT) : $(ARCHS_PGT:.a=.dummy) shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_PGT) || ( \
		echo Link $@ ; \
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_PGT) $(PGT_SYS_LIBS) $(EXECLIBS)"; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_PGT) $(PGT_SYS_LIBS) $(EXECLIBS); \
		)


#***********************************	arb_wetc **************************************
WETC = bin/arb_wetc
ARCHS_WETC = \
		WETC/WETC.a \
		SL/HELIX/HELIX.a \
		SL/FILTER/FILTER.a \
		XML/XML.a \

$(WETC): $(ARCHS_WETC:.a=.dummy) shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_WETC) $(GUI_LIBS) || ( \
		echo Link $@ ; \
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_WETC) $(GUI_LIBS) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_WETC) $(GUI_LIBS) $(EXECLIBS) ; \
		)

#***********************************	arb_dist **************************************
DIST = bin/arb_dist
ARCHS_DIST = \
		$(ARCHS_CLIENT_PROBE) \
		$(ARCHS_AP_TREE) \
		CONSENSUS_TREE/CONSENSUS_TREE.a \
		DIST/DIST.a \
		EISPACK/EISPACK.a \
		SERVERCNTRL/SERVERCNTRL.a \
		SL/GUI_ALIVIEW/GUI_ALIVIEW.a \
		SL/HELIX/HELIX.a \
		SL/MATRIX/MATRIX.a \
		SL/NEIGHBOURJOIN/NEIGHBOURJOIN.a \
		XML/XML.a \

$(DIST): $(ARCHS_DIST:.a=.dummy) shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_DIST) $(GUI_LIBS) || ( \
		echo Link $@ ; \
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_DIST) $(GUI_LIBS) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_DIST) $(GUI_LIBS) $(EXECLIBS) ; \
		)

#***********************************	arb_pars **************************************
PARSIMONY = bin/arb_pars
ARCHS_PARSIMONY = \
		$(ARCHS_AP_TREE) \
		$(ARCHS_CLIENT_NAMES) \
		PARSIMONY/PARSIMONY.a \
		SERVERCNTRL/SERVERCNTRL.a \
		SL/AW_NAME/AW_NAME.a \
		SL/GUI_ALIVIEW/GUI_ALIVIEW.a \
		SL/HELIX/HELIX.a \
		SL/TRANSLATE/TRANSLATE.a \
		SL/TREEDISP/TREEDISP.a \
		XML/XML.a \

$(PARSIMONY): $(ARCHS_PARSIMONY:.a=.dummy) shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_PARSIMONY) $(GUI_LIBS) || ( \
		echo Link $@ ; \
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_PARSIMONY) $(GUI_LIBS) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_PARSIMONY) $(GUI_LIBS) $(EXECLIBS) ; \
		)

#*********************************** arb_convert_aln **************************************
CONVERT_ALN = bin/arb_convert_aln
ARCHS_CONVERT_ALN =	\
		CONVERTALN/CONVERTALN.a \
		SL/FILE_BUFFER/FILE_BUFFER.a \


$(CONVERT_ALN) :  $(ARCHS_CONVERT_ALN:.a=.dummy)
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_CONVERT_ALN) || ( \
		echo Link $@ ; \
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_CONVERT_ALN) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARBDB_LIB) $(ARCHS_CONVERT_ALN) $(EXECLIBS) ; \
		)

#*********************************** arb_treegen **************************************
TREEGEN = bin/arb_treegen
ARCHS_TREEGEN =	\
		TREEGEN/TREEGEN.a \

$(TREEGEN) :  $(ARCHS_TREEGEN:.a=.dummy)
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_TREEGEN) || ( \
		echo Link $@ ; \
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_TREEGEN) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_TREEGEN) $(EXECLIBS) ; \
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
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_NALIGNER) $(LIBS) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_NALIGNER) $(LIBS) $(EXECLIBS) \
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
		SL/FILTER/FILTER.a \
		SL/MATRIX/MATRIX.a \
		XML/XML.a \

$(PHYLO): $(ARCHS_PHYLO:.a=.dummy) shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_PHYLO) $(GUI_LIBS) || ( \
		echo Link $@ ; \
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_PHYLO) $(GUI_LIBS) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_PHYLO) $(GUI_LIBS) $(EXECLIBS) ; \
		)

#***************************************************************************************
#					SERVER SECTION
#***************************************************************************************

#***********************************	arb_db_server **************************************
DBSERVER = bin/arb_db_server
ARCHS_DBSERVER = \
		DBSERVER/DBSERVER.a \
		SERVERCNTRL/SERVERCNTRL.a \
		PROBE_COM/client.a \

$(DBSERVER): $(ARCHS_DBSERVER:.a=.dummy) shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_DBSERVER) $(ARBDB_LIB) || ( \
		echo Link $@ ; \
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_DBSERVER) $(ARBDB_LIB) $(SYSLIBS) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_DBSERVER) $(ARBDB_LIB) $(SYSLIBS) $(EXECLIBS) ; \
		)

#***********************************	arb_pt_server **************************************
PROBE = bin/arb_pt_server
ARCHS_PROBE_COMMON = \
		PROBE_COM/server.a \
		SERVERCNTRL/SERVERCNTRL.a \
		SL/HELIX/HELIX.a \

ARCHS_PROBE_LINK = \
		$(ARCHS_PROBE_COMMON) \
		$(ARCHS_PT_SERVER_LINK) \

ARCHS_PROBE_DEPEND = \
		$(ARCHS_PROBE_COMMON) \
		$(ARCHS_PT_SERVER) \

$(PROBE): $(ARCHS_PROBE_DEPEND:.a=.dummy) shared_libs 
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_PROBE_LINK) $(ARBDB_LIB) $(ARCHS_CLIENT_PROBE) || ( \
		echo Link $@ ; \
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_PROBE_LINK) $(ARBDB_LIB) $(ARCHS_CLIENT_PROBE) $(SYSLIBS) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_PROBE_LINK) $(ARBDB_LIB) $(ARCHS_CLIENT_PROBE) $(SYSLIBS) $(EXECLIBS) ; \
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
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_NAMES) $(ARBDB_LIB) $(ARCHS_CLIENT_NAMES) $(SYSLIBS) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_NAMES) $(ARBDB_LIB) $(ARCHS_CLIENT_NAMES) $(SYSLIBS) $(EXECLIBS) ; \
		)

#***********************************	OTHER EXECUTABLES   ********************************************

ALIV3 = bin/aliv3
ARCHS_ALIV3 = \
		ALIV3/ALIV3.a \
		SL/HELIX/HELIX.a \

$(ALIV3): $(ARCHS_ALIV3:.a=.dummy) shared_libs
	@echo $(SEP) Link $@
	$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_ALIV3) $(ARBDB_LIB) $(SYSLIBS) $(EXECLIBS)

#***********************************	SHARED LIBRARIES SECTION  **************************************

shared_libs: db aw awt
		@echo -------------------- Updating shared libraries
		$(MAKE) libs

addlibs:
	(perl $(ARBHOME)/SOURCE_TOOLS/provide_libs.pl \
				"arbhome=$(ARBHOME)" \
				"opengl=$(OPENGL)" \
				"link_static=$(LINK_STATIC)" \
	)

libs:   lib/libARBDB.$(SHARED_LIB_SUFFIX) \
	lib/libWINDOW.$(SHARED_LIB_SUFFIX) \
	lib/libAWT.$(SHARED_LIB_SUFFIX)

lib/lib%.$(SHARED_LIB_SUFFIX): LIBLINK/lib%.$(SHARED_LIB_SUFFIX)
	cp $< $@

#***************************************************************************************
#			Recursive calls to sub-makefiles
#***************************************************************************************

include SOURCE_TOOLS/export2sub

%.depends:
	@cp -p $(@D)/Makefile $(@D)/Makefile.old # save old Makefile
	@$(MAKE) -C $(@D) -r \
		"AUTODEPENDS=1" \
		"MAIN=nothing" \
		"cflags=noCflagsHere_use_MAKEDEPENDFLAGS" \
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
		"MAIN=nothing" \
		"cflags=noCflags" \
		proto

%.clean:
	@$(MAKE) -C $(@D) \
		"AUTODEPENDS=0" \
		"MAIN=nothing" \
		"cflags=noCflags" \
		clean

# rule to generate main target (normally a library):
%.dummy:
	@( export ID=$$$$; \
	(( \
	    echo "$(SEP) Make everything in $(@D)"; \
	    $(MAKE) -C $(@D) -r \
		"AUTODEPENDS=1" \
		"MAIN = $(@F:.dummy=.a)" \
		"cflags = $(cflags) -DIN_ARB_$(subst /,_,$(@D))" \
	) >$(@D).$$ID.log 2>&1 && (cat $(@D).$$ID.log;rm $(@D).$$ID.log)) || (cat $(@D).$$ID.log;rm $(@D).$$ID.log;false))

# Additional dependencies for subtargets:

comtools: $(ARCHS_MAKEBIN:.a=.dummy)

PROBE_COM/PROBE_COM.dummy : comtools
PROBE_COM/server.dummy : comtools
PROBE_COM/client.dummy : comtools

NAMES_COM/NAMES_COM.dummy : comtools
NAMES_COM/server.dummy : comtools
NAMES_COM/client.dummy : comtools

com_probe: PROBE_COM/PROBE_COM.dummy 
com_names: NAMES_COM/NAMES_COM.dummy
com_all: com_probe com_names

TOOLS/TOOLS.dummy : shared_libs SERVERCNTRL/SERVERCNTRL.dummy com_probe

AWTC/AWTC.dummy :   			com_names com_probe

NAMES/NAMES.dummy : 			com_names
SL/AW_NAME/AW_NAME.dummy : 		com_names

PROBE/PROBE.dummy : 			com_probe
MULTI_PROBE/MULTI_PROBE.dummy : 	com_probe
PROBE_DESIGN/PROBE_DESIGN.dummy : 	com_probe
NALIGNER/NALIGNER.dummy : 		com_probe

ARB_GDE/ARB_GDE.dummy : 		proto_tools


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
		@echo '  db     ARB database'
		@echo '  aw     GUI lib'
		@echo '  awt    GUI toolkit'
		@echo '  awtc   general purpose library'
		@echo '  awti   import/export library'
		@echo '  mp     multi probe library'
		@echo '  ge     genome library'
		@echo '  pd     probe design lib'
		@echo '  prd    primer design lib'
		@echo ''
		@echo ' other targets:'
		@echo ''
		@echo '  help   recompile help files'
		@echo '  tools  make small tools used by arb'
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
	@echo "Remove some links (doxygen crashes otherwise):"
	find . \( -name "AISC" -o -name "C" -o -name "GDEHELP" \) -type l -exec rm {} \;
	doxygen
	$(MAKE) forcelinks

mbin:	$(ARCHS_MAKEBIN:.a=.dummy)

com:	$(ARCHS_COMMUNICATION:.a=.dummy)

help:   HELP_SOURCE/HELP_SOURCE.dummy

HELP_SOURCE/HELP_SOURCE.dummy: xml menus# need to create some files in GDE-subtree first

db:	ARBDB/libARBDB.dummy
aw:	WINDOW/libWINDOW.dummy
awt:	AWT/libAWT.dummy
awtc:	AWTC/AWTC.dummy
awti:	AWTI/AWTI.dummy

mp: 	MULTI_PROBE/MULTI_PROBE.dummy
mg:	MERGE/MERGE.dummy
ge: 	GENOM/GENOM.dummy
prd: 	PRIMER_DESIGN/PRIMER_DESIGN.dummy

nt:	menus $(NTREE)

nal:	$(NALIGNER)
a3:	$(ALIV3)

di:	$(DIST)
ph:	$(PHYLO)
pa:	$(PARSIMONY)
tg:	$(TREEGEN)

3d:	RNA3D/RNA3D.dummy
gl:	GL/GL.dummy
sl:	NAMES_COM/NAMES_COM.dummy
	$(MAKE) SL/SL.dummy

ds:	$(DBSERVER)
pt:	$(PROBE)
pst: 	PROBE_SET/PROBE_SET.dummy
pd:	PROBE_DESIGN/PROBE_DESIGN.dummy
na:	$(NAMES)
sq:	SEQ_QUALITY/SEQ_QUALITY.dummy

sec:	SECEDIT/SECEDIT.dummy

e4:	$(EDIT4) wetc help readseq menus 
gi:	GENOM_IMPORT/GENOM_IMPORT.dummy
wetc:	$(WETC)

pgt:	$(PGT)
xml:	XML/XML.dummy
xmlin:  XML_IMPORT/XML_IMPORT.dummy# broken
templ:	TEMPLATES/TEMPLATES.dummy
stat:   STAT/STAT.dummy $(NTREE) $(EDIT4)
fa:	SL/FAST_ALIGNER/FAST_ALIGNER.dummy

#********************************************************************************

up: checks
	$(MAKE) links
	$(MAKE) -k up_internal

up_internal: depends proto tags valgrind_update

#********************************************************************************

touch:
	SOURCE_TOOLS/touch_modified.pl

modified: touch
	$(MAKE) all

#********************************************************************************

depends:
	$(MAKE) comtools 
	@echo "$(SEP) Partially build com interface"
	$(MAKE) PROBE_COM/PROBE_COM.depends
	$(MAKE) NAMES_COM/NAMES_COM.depends
	@echo $(SEP) Updating dependencies
	$(MAKE) $(ARCHS:.a=.depends) \
			HELP_SOURCE/HELP_SOURCE.depends \

depend: depends

proto_tools:
	@echo $(SEP) Building prototyper 
	$(MAKE) AISC_MKPTPS/AISC_MKPTPS.dummy

#proto: proto_tools TOOLS/TOOLS.dummy 
proto: proto_tools
	@echo $(SEP) Updating prototypes
	$(MAKE) \
		AISC/AISC.proto \
		ARBDB/ARBDB.proto \
		ARB_GDE/ARB_GDE.proto \
		CONVERTALN/CONVERTALN.proto \
		NTREE/NTREE.proto \
		$(ARCHS_PT_SERVER:.a=.proto) \
		SERVERCNTRL/SERVERCNTRL.proto \
		AISC_COM/AISC_COM.proto \
		GDE/GDE.proto \
		SL/SL.proto \

#********************************************************************************

valgrind_update:
	@echo $(SEP) Updating for valgrind
	$(MAKE) -C SOURCE_TOOLS valgrind_update

#********************************************************************************

TAGFILE=TAGS
TAGFILE_TMP=TAGS.tmp

tags:
	@echo $(SEP) Updating tags
	$(MAKE) tags_$(MACH)
	mv $(TAGFILE_TMP) $(TAGFILE)

tags_LINUX: tags_ctags

tags_etags:
# first search class definitions
	etags -f $(TAGFILE_TMP)          --language=none "--regex=/^[ \t]*class[ \t]+\([^ \t]+\)/" `find . -name '*.[ch]xx' -type f`
	etags -f $(TAGFILE_TMP) --append --language=none "--regex=/\([^ \t]+\)::/" `find . -name '*.[ch]xx' -type f`
# then append normal tags (headers first)
	etags -f $(TAGFILE_TMP) --append --members ARBDB/*.h `find . -name '*.[h]xx' -type f`
	etags -f $(TAGFILE_TMP) --append ARBDB/*.c `find . -name '*.[c]xx' -type f`

tags_ctags:
	ctags -f $(TAGFILE_TMP)    -e --c-types=cdt --sort=no `find . \( -name '*.[ch]xx' -o -name "*.[ch]" \) -type f | grep -v -i perl5`
	ctags -f $(TAGFILE_TMP) -a -e --c-types=f-tvx --sort=no `find . \( -name '*.[ch]xx' -o -name "*.[ch]" \) -type f | grep -v -i perl5`

#********************************************************************************

LINKSTAMP=SOURCE_TOOLS/generate_all_links.stamp

links: $(LINKSTAMP)

forcelinks:
	-rm $(LINKSTAMP)
	$(MAKE) links

$(LINKSTAMP): SOURCE_TOOLS/generate_all_links.sh
	SOURCE_TOOLS/generate_all_links.sh
	touch $(LINKSTAMP)

redo_links:
	find . -path './lib' -prune -o -type l -exec rm {} \;
	@-rm $(LINKSTAMP)
	$(MAKE) links

gde:		GDE/GDE.dummy
GDE:		gde
agde: 		ARB_GDE/ARB_GDE.dummy
tools:		shared_libs SL/SL.dummy TOOLS/TOOLS.dummy

convert:	$(CONVERT_ALN)
readseq:	READSEQ/READSEQ.dummy

#***************************************************************************************
#			Some user commands
#***************************************************************************************

menus: binlink
	@(( \
		echo $(SEP) Make everything in GDEHELP; \
		$(MAKE) -C GDEHELP -r "PP=$(PP)" all \
	) > GDEHELP.log 2>&1 && (cat GDEHELP.log;rm GDEHELP.log)) || (cat GDEHELP.log;rm GDEHELP.log;false)

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


preplib:
	(cd lib;$(MAKE) all)

# ---------------------------------------- perl

perl: tools
	@echo $(SEP) Make everything in PERL2ARB
	@$(MAKE) -C PERL2ARB -r -f Makefile.main \
		"AUTODEPENDS=1" \
		"dflags=$(dflags)" \
		"cross_cflags=$(cross_cflags) $(dflags)" \
		"cross_lflags=$(cross_lflags)" \
		all

testperlscripts: perl 
	@$(MAKE) -C PERL_SCRIPTS/test test

perl_clean:
	@$(MAKE) -C PERL2ARB -r -f Makefile.main \
		"AUTODEPENDS=0" \
		clean

# ----------------------------------------

CLOC=cloc-1.08.pl
CLOCFLAGS=--no3 --quiet --progress-rate=0
CLOCARB=--exclude-dir=GDE .
CLOCEXT=GDE
CLOCCODE=--read-lang-def=$(ARBHOME)/SOURCE_TOOLS/arb.cloc.code.def
CLOCREST=--read-lang-def=$(ARBHOME)/SOURCE_TOOLS/arb.cloc.rest.def
CLOCFILT=tail --lines=+4

cloc:
	@echo 'Arb code:'
	@$(CLOC) $(CLOCFLAGS) $(CLOCCODE) $(CLOCARB) | $(CLOCFILT)
	@echo 'Arb rest:'
	@$(CLOC) $(CLOCFLAGS) $(CLOCREST) $(CLOCARB) | $(CLOCFILT)
	@echo 'External code:'
	@$(CLOC) $(CLOCFLAGS) $(CLOCCODE) $(CLOCEXT) | $(CLOCFILT)
	@echo 'External rest:'
	@$(CLOC) $(CLOCFLAGS) $(CLOCREST) $(CLOCEXT) | $(CLOCFILT)

# ---------------------------------------- check ressources

check_res:
	$(ARBHOME)/SOURCE_TOOLS/check_ressources.pl

# ---------------------------------------- cleaning

rmbak:
	@echo "Cleanup:"
	@find . \( -name '*%' -o -name '*.bak' -o -name 'core' \
		   -o -name 'infile' -o -name treefile -o -name outfile \
		   -o -name 'gde*_?' -o -name '*~' \) \
	        -exec rm -v {} \;

binclean:
	@echo Cleaning bin directory
	find bin -type l -exec rm {} \;
	find bin -type f \! \( -name ".cvsignore" -o -name "Makefile" -o -path "bin/CVS/*" -o -path "bin/.svn/*" \) -exec rm {} \;
	cd bin;make all

libclean:
	find $(ARBHOME) -type f \( -name '*.a' ! -type l \) -exec rm -f {} \;

objclean:
	find $(ARBHOME) -type f \( -name '*.o' ! -type l \) -exec rm -f {} \;

clean2: $(ARCHS:.a=.clean) \
		GDEHELP/GDEHELP.clean \
		HELP_SOURCE/HELP_SOURCE.clean \
		SOURCE_TOOLS/SOURCE_TOOLS.clean \
		UNIT_TESTER/UNIT_TESTER.clean \
		bin/bin.clean \
		perl_clean
	rm -f *.last_gcc

# links are needed for cleanup
clean: redo_links
	$(MAKE) clean2
	$(MAKE) clean_coverage

# 'relocated' is about 50% faster than 'rebuild'
reloc_clean: links
	@echo "---------------------------------------- Relocation cleanup"
	$(MAKE) \
		perl_clean \
		GDEHELP/GDEHELP.clean \
		HELP_SOURCE/genhelp/genhelp.clean \
		binclean \
		libclean \
		objclean

relocated: links
	$(MAKE) reloc_clean
	@echo "---------------------------------------- and remake"
	$(MAKE) all

# -----------------------------------

rebuild: clean
	$(MAKE) all

relink: binclean libclean
	$(MAKE) all

tarfile: rebuild
	$(MAKE) addlibs 
	util/arb_compress

tarfile_quick: all
	$(MAKE) addlibs 
	util/arb_compress

save: sourcetarfile 

patch:
	SOURCE_TOOLS/arb_create_patch.sh arbPatch

# test early whether save will work
testsave:
	@util/arb_srclst.pl >/dev/null

sourcetarfile: rmbak
	util/arb_save

save2: rmbak
	util/arb_save ignore

save_test: rmbak
	@echo "Testing source list.."
	@util/arb_srclst.pl > /dev/null

rel_minor:
	touch SOURCE_TOOLS/inc_minor.stamp
	$(MAKE) do_release

rel_major:
	touch SOURCE_TOOLS/inc_major.stamp
	$(MAKE) do_release

do_release: 
	@echo Building release
	@echo PATH=$(PATH)
	@echo ARBHOME=$(ARBHOME)
	-rm arb.tgz arbsrc.tgz
	$(MAKE) testsave
	$(MAKE) templ # auto upgrades version early
	$(MAKE) tarfile sourcetarfile

release_quick:
	-rm arb.tgz arbsrc.tgz
	$(MAKE) tarfile_quick sourcetarfile

# --------------------------------------------------------------------------------

# basic arb libraries
arbbasic: links preplib
		$(MAKE) arbbasic2

arbbasic2: templ mbin com sl $(GL)

# needed arb applications
arbapplications: nt pa e4 wetc pt na nal di ph ds pgt

# optionally things (no real harm for ARB if any of them fails):
arbxtras: tg pst a3 xmlin 

tryxtras:
	@echo $(SEP)
	@( $(MAKE) arbxtras || ( \
		echo $(SEP) ;\
		echo "One of the optional tools failed to build (see error somewhere above)" ;\
		echo "ARB will work nevertheless!" ) )

arb: arbbasic
	$(MAKE) shared_libs arbapplications help

# --------------------------------------------------------------------------------
# unit testing 
# @@@ work in progress
#
# goal is to automatically test all libraries/executables using TESTED_UNITS_AUTO
#
# currently not all test executables link w/o error
# (see UNITS_WORKING, UNITS_UNTESTABLE_ATM and UNITS_NEED_FIX)

# define RNA3D/RNA3D.test
RNA3D_TEST := $(subst .a,.test,$(RNA3D_LIB))

TESTED_UNITS_AUTO = $(ARCHS:.a=.test)

UNITS_WORKING = \
	ISLAND_HOPPING/ISLAND_HOPPING.test \
	SL/ALIVIEW/ALIVIEW.test \
	SL/AP_TREE/AP_TREE.test \
	SL/ARB_TREE/ARB_TREE.test \
	SL/AW_HELIX/AW_HELIX.test \
	SL/AW_NAME/AW_NAME.test \
	SL/DB_SCANNER/DB_SCANNER.test \
	SL/FILE_BUFFER/FILE_BUFFER.test \
	SL/FILTER/FILTER.test \
	SL/GUI_ALIVIEW/GUI_ALIVIEW.test \
	ARB_GDE/ARB_GDE.test \
	SL/HELIX/HELIX.test \
	SL/MATRIX/MATRIX.test \
	SL/NEIGHBOURJOIN/NEIGHBOURJOIN.test \
	SL/REGEXPR/REGEXPR.test \
	SL/SEQUENCE/SEQUENCE.test \
	SL/TRANSLATE/TRANSLATE.test \
	SL/TREEDISP/TREEDISP.test \
	SL/TREE_READ/TREE_READ.test \
	XML/XML.test \
	SL/TREE_WRITE/TREE_WRITE.test \
	ALIV3/ALIV3.test \
	CONSENSUS_TREE/CONSENSUS_TREE.test \
	EISPACK/EISPACK.test \
	DIST/DIST.test \
	NALIGNER/NALIGNER.test \
	GL/glAW/libglAW.test \
	GL/glpng/libglpng_arb.test \
	SEQ_QUALITY/SEQ_QUALITY.test \
	STAT/STAT.test \
	WETC/WETC.test \
	ptpan/PROBE.test \
	PHYLO/PHYLO.test \
	PROBE_DESIGN/PROBE_DESIGN.test \
	PRIMER_DESIGN/PRIMER_DESIGN.test \
	PGT/PGT.test \
	PARSIMONY/PARSIMONY.test \
	TREEGEN/TREEGEN.test \
	NTREE/NTREE.test \
	NAMES/NAMES.test \
	PROBE/PROBE.test \
	DBSERVER/DBSERVER.test \
	SECEDIT/SECEDIT.test \
	$(RNA3D_TEST) \
	SERVERCNTRL/SERVERCNTRL.test \
	AWT/AWT.test \

# untestable units

UNITS_TRY_FIX = \

UNITS_NEED_FIX = \
	GENOM_IMPORT/GENOM_IMPORT.test \
	GENOM/GENOM.test \
	AWTI/AWTI.test \

UNITS_UNTESTABLE_ATM = \
	PROBE_SET/PROBE_SET.test \
	XML_IMPORT/XML_IMPORT.test \

# for the moment, put all units containing tests into UNITS_TESTED:
UNITS_TESTED = \
	AISC_MKPTPS/mkptypes.test \
	ARBDB/ARBDB.test \
	AWTC/AWTC.test \
	EDIT4/EDIT4.test \
	MERGE/MERGE.test \
	MULTI_PROBE/MULTI_PROBE.test \
	SERVERCNTRL/SERVERCNTRL.test \
	SL/FAST_ALIGNER/FAST_ALIGNER.test \
	SL/PRONUC/PRONUC.test \
	TOOLS/arb_probe.test \
	WINDOW/WINDOW.test \
	HELP_SOURCE/arb_help2xml.test \
	CONVERTALN/CONVERTALN.test \
	SL/SEQIO/SEQIO.test \

TESTED_UNITS_MANUAL = \
	$(UNITS_TRY_FIX) \
	$(UNITS_TESTED) \

#	$(UNITS_WORKING)

#TESTED_UNITS = $(TESTED_UNITS_AUTO)
TESTED_UNITS = $(TESTED_UNITS_MANUAL)

# ----------------------------------------

TEST_LOG_DIR = UNIT_TESTER/logs
TEST_RUN_SUITE=$(MAKE) $(NODIR) -C UNIT_TESTER -f Makefile.suite -r

%.test:
	-@( export ID=$$$$; mkdir -p $(TEST_LOG_DIR); \
	( \
	    $(MAKE) -C UNIT_TESTER -f Makefile.test -r \
		"UNITDIR=$(@D)" \
		"UNITLIBNAME=$(@F:.test=)" \
		"COVERAGE=$(COVERAGE)" \
		"cflags=$(cflags)" \
		runtest \
	) >$(TEST_LOG_DIR)/$(@F).log 2>&1 ; cat $(TEST_LOG_DIR)/$(@F).log)

test_base: $(UNIT_TESTER_LIB:.a=.dummy)

clean_coverage_results:
	find . \( -name "*.gcda" -o -name "*.gcov" -o -name "*.cov" \) -exec rm {} \;

clean_coverage: clean_coverage_results
	find . \( -name "*.gcno" \) -exec rm {} \;


unit_tests: test_base clean_coverage_results
	@$(TEST_RUN_SUITE) init
	@echo "fake[1]: Entering directory \`$(ARBHOME)/UNIT_TESTER'"
	$(MAKE) $(NODIR) $(TESTED_UNITS)
	@echo "fake[1]: Leaving directory \`$(ARBHOME)/UNIT_TESTER'"
	@$(TEST_RUN_SUITE) cleanup

ut:
ifeq ($(UNIT_TESTS),1)
	@echo $(MAKE) unit_tests
	@$(MAKE) unit_tests
else
	@echo "Not compiled with unit tests"
endif


aut:
	@$(TEST_RUN_SUITE) unskip
	$(MAKE) ut

# --------------------------------------------------------------------------------

TIMELOG=$(ARBHOME)/arb_time.log
TIMEARGS=--append --output=$(TIMELOG) --format=" %E(%S+%U) %P [%C]"
TIMECMD=/usr/bin/time $(TIMEARGS)

build:
	$(MAKE) links
	$(MAKE) com
	$(MAKE) arb
	$(MAKE) libs
	$(MAKE) convert tools gde readseq
	$(MAKE) binlink
	$(MAKE) perl
	-$(MAKE) tryxtras
ifeq ("$(DEVELOPER)","SAVETEST")
	$(MAKE) save_test
endif

all: checks
	@echo "Build time" > $(TIMELOG)
	@echo $(MAKE) build
	@$(TIMECMD) $(MAKE) build
	@echo $(SEP)
	@echo "made 'all' with success."
	@echo "to start arb enter 'arb'"
ifeq ($(UNIT_TESTS),1)
	@echo $(MAKE) unit_tests
	@$(TIMECMD) $(MAKE) unit_tests
endif
	@cat $(TIMELOG)
	@rm $(TIMELOG)
