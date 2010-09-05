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
# $(MACH)               name of the machine (LINUX,SUN4,SUN5,HP,SGI or DIGITAL; see config.makefile)
# DEBUG                 compiles the DEBUG sections
# DEBUG_GRAPHICS        all X-graphics are flushed immediately (for debugging)
# ARB_64=0/1            1=>compile 64 bit version
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
	4.3 4.3.1 4.3.2 4.3.3 \

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

	POST_COMPILE := 2>&1 | $(ARBHOME)/SOURCE_TOOLS/postcompile.pl

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

ifeq ($(ARB_64),1)
	dflags += -DARB_64 #-fPIC 
	lflags +=
	shared_cflags += -fPIC

	ifeq ($(BUILDHOST_64),1)
#		build 64-bit ARB version on 64-bit host
		CROSS_LIB:=# empty = autodetect below
		ifdef DARWIN
			cflags += -arch x86_64
			lflags += -arch x86_64
		endif
	else
#		build 64-bit ARB version on 32-bit host
		CROSS_LIB:=/lib64
		cflags += -m64
		lflags += -m64 -m elf_x86_64
	endif
else
	ifeq ($(BUILDHOST_64),1)
#		build 32-bit ARB version on 64-bit host
		CROSS_LIB:=# empty = autodetect below
		cflags += -m32
		lflags += -m32 -m elf_i386 
	else
#		build 32-bit ARB version on 32-bit host
		CROSS_LIB:=/lib
	endif
endif

ifeq ('$(CROSS_LIB)','')
# autodetect libdir
	ifeq ($(ARB_64),1)
		CROSS_LIB:=`(test -d /lib64 && echo lib64) || echo lib`
	else
		CROSS_LIB:=`(test -d /lib32 && echo lib32) || echo lib`
	endif
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
	XINCLUDES := -I/sw/include -I$(OSX_SDK)/usr/X11/include -I$(OSX_SDK)/usr/include/krb5
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
	GL_LIB := -lGL -L$(ARBHOME)/GL/glAW -lglAW

 ifdef DEBIAN
	GL_LIB += -lpthread
 endif

GL_PNGLIBS := -L$(ARBHOME)/GL/glpng -lglpng_arb -lpng

 ifdef DARWIN
	GLEWLIB := -L/usr/lib -lGLEW -L$(OSX_SDK)/usr/X11/lib -lGLw
	GLUTLIB := -L$(XHOME)/lib -lglut
 else 
	GLEWLIB := -lGLEW -lGLw
	GLUTLIB := -lglut
 endif

GL_LIBS := $(GL_LIB) $(GLEWLIB) $(GLUTLIB) $(GL_PNGLIBS)

#XLIBS += $(GL_LIB)

else
# OPENGL=0

GL_LIBS:=# no opengl -> no libs
GL:=# don't build ARB openGL libs

endif

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
LINK_SHARED_LIB := $(GPP) $(lflags) -shared -o# link shared lib
endif

# other used tools

CTAGS := etags
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
		@echo ' modified     - rebuild files modified in svn checkout (touches files!)'
		@echo ''
		@echo 'Internal maintenance:'
		@echo ''
		@echo ' release     - build a release (increases minor version number)'
		@echo ' stable      - like "release", but increase major version number'
		@echo ' tarfile     - make rebuild and create arb version tarfile ("tarfile_quick" to skip rebuild)'
		@echo ' save        - save all basic ARB sources into arbsrc_DATE'
		@echo ' rtc_patch   - create LIBLINK/libRTC8M.so (SOLARIS ONLY)'
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
ARBDBPP_LIB=-lARBDBPP

LIBS = $(ARBDB_LIB) $(SYSLIBS)
GUI_LIBS = $(LIBS) -lAW -lAWT $(XLIBS)

LIBPATH = -L$(ARBHOME)/LIBLINK

DEST_LIB = lib
DEST_BIN = bin

AINCLUDES := -I. -I$(ARBHOME)/INCLUDE $(XINCLUDES)
CPPINCLUDES := -I. -I$(ARBHOME)/INCLUDE $(XINCLUDES)
MAKEDEPENDFLAGS := -- $(cflags) -- -DARB_OPENGL -I. -Y$(ARBHOME)/INCLUDE

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
			ARBDB2/libARBDB.a \
			ARBDBPP/libARBDBPP.a \
			ARBDBS/libARBDB.a \
			ARB_GDE/ARB_GDE.a \
			AWT/libAWT.a \
			AWTC/AWTC.a \
			AWTI/AWTI.a \
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
			WETC/WETC.a \
			WINDOW/libAW.a \
			XML/XML.a \

ARCHS_CLIENT_PROBE = PROBE_COM/client.a
ARCHS_CLIENT_NAMES = NAMES_COM/client.a
ARCHS_MAKEBIN = AISC_MKPTPS/dummy.a AISC/dummy.a

ARCHS_COMMUNICATION =	NAMES_COM/server.a \
			PROBE_COM/server.a

# communication libs need aisc and aisc_mkpts:
$(ARCHS_COMMUNICATION:.a=.dummy) : $(ARCHS_MAKEBIN:.a=.dummy)

#***************************************************************************************
#		Individual Programs Section
#***************************************************************************************

#***********************************	arb_ntree **************************************
NTREE = bin/arb_ntree
ARCHS_NTREE = \
		$(ARCHS_CLIENT_PROBE) \
		ARB_GDE/ARB_GDE.a \
		AWTC/AWTC.a \
		AWTI/AWTI.a \
		GENOM/GENOM.a \
		GENOM_IMPORT/GENOM_IMPORT.a \
		MERGE/MERGE.a \
		MULTI_PROBE/MULTI_PROBE.a \
		NTREE/NTREE.a \
		PRIMER_DESIGN/PRIMER_DESIGN.a \
		PROBE_DESIGN/PROBE_DESIGN.a \
		SEQ_QUALITY/SEQ_QUALITY.a \
		SERVERCNTRL/SERVERCNTRL.a \
		SL/AW_NAME/AW_NAME.a \
		SL/DB_SCANNER/DB_SCANNER.a \
		SL/FILE_BUFFER/FILE_BUFFER.a \
		SL/HELIX/HELIX.a \
		SL/REGEXPR/REGEXPR.a \
		SL/TREE_READ/TREE_READ.a \
		SL/TREE_WRITE/TREE_WRITE.a \
		STAT/STAT.a \
		XML/XML.a \

$(NTREE): $(ARCHS_NTREE:.a=.dummy) NAMES_COM/server.dummy shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_NTREE) $(GUI_LIBS) || ( \
		echo Link $@ ; \
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_NTREE) $(GUI_LIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_NTREE) $(GUI_LIBS)  \
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
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_EDIT) -lARBDBPP $(GUI_LIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_EDIT) -lARBDBPP $(GUI_LIBS) ; \
		)

#***********************************	arb_edit4 **************************************
EDIT4 = bin/arb_edit4

ARCHS_EDIT4 := \
		NAMES_COM/client.a \
		AWTC/AWTC.a \
		EDIT4/EDIT4.a \
		SECEDIT/SECEDIT.a \
		SERVERCNTRL/SERVERCNTRL.a \
		STAT/STAT.a \
		ARB_GDE/ARB_GDE.a \
		ISLAND_HOPPING/ISLAND_HOPPING.a \
		SL/FAST_ALIGNER/FAST_ALIGNER.a \
		SL/HELIX/HELIX.a \
		SL/AW_HELIX/AW_HELIX.a \
		SL/AW_NAME/AW_NAME.a \
		SL/FILE_BUFFER/FILE_BUFFER.a \
		XML/XML.a \

ifeq ($(OPENGL),1)
ARCHS_EDIT4 += RNA3D/RNA3D.a
endif
LIBS_EDIT4 := $(GL_LIBS)

$(EDIT4): $(ARCHS_EDIT4:.a=.dummy) shared_libs $(GL)
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_EDIT4) $(GUI_LIBS) || ( \
		echo Link $@ ; \
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_EDIT4) $(GUI_LIBS) $(LIBS_EDIT4)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_EDIT4) $(GUI_LIBS) $(LIBS_EDIT4) \
		)

#***********************************	arb_pgt **************************************

PGT = bin/arb_pgt
ARCHS_PGT = \
		PGT/PGT.a \

PGT_SYS_LIBS=$(XLIBS) $(TIFFLIBS) $(LIBS)

$(PGT) : $(ARCHS_PGT:.a=.dummy) shared_libs
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_PGT) || ( \
		echo Link $@ ; \
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_PGT) $(PGT_SYS_LIBS)"; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_PGT) $(PGT_SYS_LIBS); \
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
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_WETC) $(GUI_LIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_WETC) $(GUI_LIBS) ; \
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
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_DIST) $(GUI_LIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_DIST) $(GUI_LIBS) ; \
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
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_PARSIMONY) $(GUI_LIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_PARSIMONY) $(GUI_LIBS) ; \
		)

#*********************************** arb_treegen **************************************
TREEGEN = bin/arb_treegen
ARCHS_TREEGEN =	\
		TREEGEN/TREEGEN.a \

$(TREEGEN) :  $(ARCHS_TREEGEN:.a=.dummy)
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_TREEGEN) || ( \
		echo Link $@ ; \
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_TREEGEN)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_TREEGEN) ; \
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
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_NALIGNER) $(LIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_NALIGNER) $(LIBS) \
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
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_PHYLO) $(GUI_LIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_PHYLO) $(GUI_LIBS) ; \
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
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_DBSERVER) $(ARBDB_LIB) $(SYSLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_DBSERVER) $(ARBDB_LIB) $(SYSLIBS) ; \
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
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_PROBE_LINK) $(ARBDB_LIB) $(ARCHS_CLIENT_PROBE) $(SYSLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_PROBE_LINK) $(ARBDB_LIB) $(ARCHS_CLIENT_PROBE) $(SYSLIBS) ; \
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
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_NAMES) $(ARBDB_LIB) $(ARCHS_CLIENT_NAMES) $(SYSLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_NAMES) $(ARBDB_LIB) $(ARCHS_CLIENT_NAMES) $(SYSLIBS) ; \
		)

#***********************************	OTHER EXECUTABLES   ********************************************

ALIV3 = bin/aliv3
ARCHS_ALIV3 = \
		ALIV3/ALIV3.a \
		SL/HELIX/HELIX.a \

$(ALIV3): $(ARCHS_ALIV3:.a=.dummy) shared_libs
	@echo $(SEP) Link $@
	$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_ALIV3) $(ARBDB_LIB) $(SYSLIBS)

#***********************************	SHARED LIBRARIES SECTION  **************************************

shared_libs: dball aw awt
		@echo -------------------- Updating shared libraries
		$(MAKE) libs

addlibs:
	(perl $(ARBHOME)/SOURCE_TOOLS/provide_libs.pl \
				"arbhome=$(ARBHOME)" \
				"opengl=$(OPENGL)" \
				"link_static=$(LINK_STATIC)" \
	)

libs:	lib/libARBDB.$(SHARED_LIB_SUFFIX) \
	lib/libARBDBPP.$(SHARED_LIB_SUFFIX) \
	lib/libARBDO.$(SHARED_LIB_SUFFIX) \
	lib/libAW.$(SHARED_LIB_SUFFIX) \
	lib/libAWT.$(SHARED_LIB_SUFFIX)

lib/lib%.$(SHARED_LIB_SUFFIX): LIBLINK/lib%.$(SHARED_LIB_SUFFIX)
	cp $< $@

# lib/$(MOTIF_LIBNAME):  $(MOTIF_LIBPATH)
# 	cp $< $@

#***************************************************************************************
#			Recursive calls to sub-makefiles
#***************************************************************************************

include SOURCE_TOOLS/export2sub

%.depends:
	@cp -p $(@D)/Makefile $(@D)/Makefile.old # save old Makefile
	@$(MAKE) -C $(@D) -r \
		"AUTODEPENDS=1" \
		"MAIN=nothing" \
		"cflags=noCflags" \
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
		@echo '  dball  ARB database (all versions: db dbs and db2)'
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
		doxygen

mbin:	$(ARCHS_MAKEBIN:.a=.dummy)

com:	$(ARCHS_COMMUNICATION:.a=.dummy)

help:   HELP_SOURCE/HELP_SOURCE.dummy

HELP_SOURCE/HELP_SOURCE.dummy: xml menus# need to create some files in GDE-subtree first

dball:	db dbs db2 dp
db:	ARBDB/libARBDB.dummy
dbs:	ARBDBS/libARBDB.dummy
db2:	ARBDB2/libARBDB.dummy
dp:	ARBDBPP/libARBDBPP.dummy
aw:	WINDOW/libAW.dummy
awt:	AWT/libAWT.dummy
awtc:	AWTC/AWTC.dummy
awti:	AWTI/AWTI.dummy

mp: 	MULTI_PROBE/MULTI_PROBE.dummy
mg:	MERGE/MERGE.dummy
ge: 	GENOM/GENOM.dummy
prd: 	PRIMER_DESIGN/PRIMER_DESIGN.dummy

nt:	menus $(NTREE)
ed:	$(EDIT)

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

e4:	wetc help readseq menus $(EDIT4)
gi:	GENOM_IMPORT/GENOM_IMPORT.dummy
wetc:	$(WETC)

pgt:	$(PGT)
xml:	XML/XML.dummy
xmlin:  XML_IMPORT/XML_IMPORT.dummy# broken
templ:	TEMPLATES/TEMPLATES.dummy

#********************************************************************************

up: checks
	$(MAKE) links
	$(MAKE) -k up_internal

up_internal: depends proto tags valgrind_update

#********************************************************************************

modified:
	SOURCE_TOOLS/touch_modified.pl
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

tags_LINUX: tags2
tags_SUN5: tags1

tags1:
# first search class definitions
	$(CTAGS) -f $(TAGFILE_TMP)          --language=none "--regex=/^[ \t]*class[ \t]+\([^ \t]+\)/" `find . -name '*.[ch]xx' -type f`
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
tools:		SL/SL.dummy TOOLS/TOOLS.dummy
convert:	SL/FILE_BUFFER/FILE_BUFFER.dummy shared_libs
	$(MAKE) CONVERTALN/CONVERTALN.dummy

readseq:	READSEQ/READSEQ.dummy

#***************************************************************************************
#			Some user commands
#***************************************************************************************
rtc_patch:
	rtc_patch_area -so LIBLINK/libRTC8M.so

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
		all
	@$(MAKE) testperlscripts

testperlscripts: 
	@$(MAKE) -C PERL_SCRIPTS/test test

perl_clean:
	@$(MAKE) -C PERL2ARB -r -f Makefile.main \
		"AUTODEPENDS=0" \
		clean

# ----------------------------------------

wc:
	wc `find . -type f \( -name '*.[ch]' -o -name '*.[ch]xx' \) -print`

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
		bin/bin.clean \
		perl_clean
	rm -f *.last_gcc

# links are needed for cleanup
clean: links
	$(MAKE) clean2

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

release:
	touch SOURCE_TOOLS/inc_minor.stamp
	$(MAKE) do_release

stable:
	touch SOURCE_TOOLS/inc_major.stamp
	$(MAKE) do_release

do_release: 
	@echo Making release
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

# shared arb libraries
arbshared: dball aw dp awt

# needed arb applications
arbapplications: nt pa ed e4 wetc pt na nal di ph ds pgt

# optionally things (no real harm for ARB if any of them fails):
arbxtras: tg pst a3 xmlin 

tryxtras:
	@echo $(SEP)
	@( $(MAKE) arbxtras || ( \
		echo $(SEP) ;\
		echo "One of the optional tools failed to build (see error somewhere above)" ;\
		echo "ARB will work nevertheless!" ) )

arb: arbbasic
	$(MAKE) arbshared arbapplications help

all: checks
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
	@echo $(SEP)
	@echo "made 'all' with success."
	@echo "to start arb enter 'arb'"

