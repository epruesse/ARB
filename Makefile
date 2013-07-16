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
# DEVELOPER=name        special compilation (values: ANY,RELEASE,your name)
# OPENGL=0/1            whether OPENGL is available
# GTK=2/3               gtk version to use
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

SED:=$(ARBHOME)/SH/arb_sed
READLINK:=$(ARBHOME)/SH/arb_readlink

# ---------------------- [basic compiler setting]

GCC ?= $(CC)
GPP ?= $(CXX)
export GCC
export GPP

# ---------------------- compiler version detection

# supported compiler versions:

ALLOWED_GCC_4xx_VERSIONS=\
	4.2.1 \
	4.3.1 4.3.2 4.3.3 4.3.4 \
	4.4.1       4.4.3       4.4.5 4.4.6  4.4.7 \
	      4.5.2 \
	4.6.1 4.6.2 4.6.3 \
	4.7.1 4.7.2 4.7.3 \
	4.8.0 4.8.1

ALLOWED_GCC_VERSIONS=$(ALLOWED_GCC_4xx_VERSIONS)

GCC_VERSION_FOUND=$(shell SOURCE_TOOLS/arb_gcc_version.pl)
GCC_VERSION_ALLOWED=$(strip $(subst ___,,$(foreach version,$(ALLOWED_GCC_VERSIONS),$(findstring ___$(version)___,___$(GCC_VERSION_FOUND)___))))

#---------------------- split gcc version 

SPLITTED_VERSION:=$(subst ., ,$(GCC_VERSION_FOUND))

USE_GCC_MAJOR:=$(word 1,$(SPLITTED_VERSION))
USE_GCC_MINOR:=$(word 2,$(SPLITTED_VERSION))
USE_GCC_PATCHLEVEL:=$(word 3,$(SPLITTED_VERSION))

USE_GCC_452_OR_HIGHER:=
USE_GCC_47_OR_HIGHER:=
USE_GCC_48_OR_HIGHER:=

ifeq ($(USE_GCC_MAJOR),4)
 ifeq ($(USE_GCC_MINOR),5)
  ifneq ('$(findstring $(USE_GCC_PATCHLEVEL),23456789)','')
   USE_GCC_452_OR_HIGHER:=yes
  endif
 else
  ifneq ('$(findstring $(USE_GCC_MINOR),6789)','')
   USE_GCC_452_OR_HIGHER:=yes
   ifneq ($(USE_GCC_MINOR),6)
    USE_GCC_47_OR_HIGHER:=yes
    ifneq ($(USE_GCC_MINOR),7)
     USE_GCC_48_OR_HIGHER:=yes
    endif
   endif
  endif
 endif
else
 USE_GCC_452_OR_HIGHER:=yes
 USE_GCC_47_OR_HIGHER:=yes
 USE_GCC_48_OR_HIGHER:=yes
endif

#---------------------- define special directories for non standard builds

ifdef DARWIN
	OSX_FW:=/System/Library/Frameworks
	OSX_FW_OPENGL:=$(OSX_FW)/OpenGL.framework/Versions/A/Libraries
	OSX_FW_GLUT:=$(OSX_FW)/GLUT.framework/Versions/A/Libraries
	OSX_FW_IMAGEIO:=$(OSX_FW)/ApplicationServices.framework/Versions/A/Frameworks/ImageIO.framework/Versions/A/Resources
endif

#----------------------

ifdef DARWIN
	LINK_STATIC=1# link static
else
	LINK_STATIC=0# link dynamically
#	LINK_STATIC=1# link static (testing only)
endif

shared_cflags :=# flags for shared lib compilation
lflags :=# linker flags
extended_warnings :=# warning flags for C and C++-compiler
extended_cpp_warnings :=# warning flags for C++-compiler only


ifeq ($(DEBUG),0)
	dflags := -DNDEBUG# defines
	ifdef DARWIN
		cflags := -O3# compiler flags (C and C++)
	else
		cflags := -g -ggdb -O0# compiler flags (C and C++)
		lflags += -O99# linker flags
	endif
endif

ifeq ($(DEBUG),1)
	dflags := -DDEBUG

	gdb_common := -g -g3 -ggdb -ggdb3

	cflags := -O0 $(gdb_common) # (using dwarf - cant debug inlines here, incredible slow on showing variable content)
#	cflags := -O0 $(gdb_common) -gdwarf-3 # (specify explicit dwarf format)
#	cflags := -O0  $(gdb_common) -gstabs+  # using stabs+ (enable this for bigger debug session: debugs inlines, quick var inspect, BUT valgrind stops working :/)
#	cflags := -O0  $(gdb_common) -gstabs  # using stabs (same here IIRC)
#	cflags := -O2 $(gdb_common) # use this for callgrind (force inlining)
 ifndef DARWIN
	lflags += -g
 endif

# control how much you get spammed
# (please do not change default in SVN, use developer specific setting as below)
	POST_COMPILE := 2>&1 | $(ARBHOME)/SOURCE_TOOLS/postcompile.pl
#	POST_COMPILE := 2>&1 | $(ARBHOME)/SOURCE_TOOLS/postcompile.pl --original# dont modify compiler output
#	POST_COMPILE := 2>&1 | $(ARBHOME)/SOURCE_TOOLS/postcompile.pl --hide-Noncopyable-advices
#	POST_COMPILE := 2>&1 | $(ARBHOME)/SOURCE_TOOLS/postcompile.pl --show-useless-Weff++
#	POST_COMPILE := 2>&1 | $(ARBHOME)/SOURCE_TOOLS/postcompile.pl --no-warnings
#	POST_COMPILE := 2>&1 | $(ARBHOME)/SOURCE_TOOLS/postcompile.pl --only-first-error
#	POST_COMPILE := 2>&1 | $(ARBHOME)/SOURCE_TOOLS/postcompile.pl --no-warnings --only-first-error
ifeq ($(DEVELOPER),ELMAR)
	POST_COMPILE := 2>&1 | $(ARBHOME)/SOURCE_TOOLS/postcompile.pl --only-first-error
endif


# Enable extra warnings
	extended_warnings :=
	extended_cpp_warnings :=

#       C and C++ 
	extended_warnings     += -Wwrite-strings -Wunused -Wno-aggregate-return -Wshadow

#       C++ only 
	extended_cpp_warnings += -Wnon-virtual-dtor -Wreorder -Wpointer-arith -Wdisabled-optimization -Wmissing-format-attribute
	extended_cpp_warnings += -Wctor-dtor-privacy# < gcc 3
# 	extended_cpp_warnings += -Wfloat-equal# gcc 3.0

# ------- above only warnings available in 3.0

 ifneq ('$(GCC_VERSION_FOUND)','4.7.3') # g++ crashes on GenomeImport.cxx when -Weffc++ is active (bug reported)
	extended_cpp_warnings += -Weffc++# gcc 3.0.1
 endif
	extended_cpp_warnings += -Wmissing-noreturn# gcc 3.0.2
#	extended_cpp_warnings += -Wold-style-cast# gcc 3.0.4 (warn about 28405 old-style casts)
	extended_cpp_warnings += -Winit-self# gcc 3.4.0
	extended_cpp_warnings += -Wstrict-aliasing# gcc 3.4
	extended_cpp_warnings += -Wextra# gcc 3.4.0
 ifeq ('$(USE_GCC_452_OR_HIGHER)','yes')
	extended_cpp_warnings += -Wlogical-op# gcc 4.5.2
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

ifndef SHOWTODO
 ifeq ($(DEVELOPER),RALF)
	SHOWTODO:=1
 else
	SHOWTODO:=0
 endif
endif
ifeq ($(SHOWTODO),1)
	dflags += -DWARN_TODO# activate "TODO" warnings
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
		CROSS_LIB:=$(shell (test -d /lib64 && echo lib64) || echo lib)
	else
		CROSS_LIB:=$(shell (test -d /lib32 && echo lib32) || echo lib)
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
	shared_cflags += -fno-common
else
	dflags +=  $(shell getconf LFS_CFLAGS)
endif

cflags += -pipe
cflags += -fmessage-length=0# don't wrap compiler output
cflags += -fshow-column# show columns
cflags += -funit-at-a-time
cflags += -fPIC
cflags += -fno-common# link all global data into one namespace
cflags += -fstrict-aliasing# gcc 3.4
ifeq ('$(USE_GCC_48_OR_HIGHER)','yes')
 cflags += -fno-diagnostics-show-caret#gcc 4.8 (4.7.?)
endif
#cflags += -save-temps# uncomment to see preprocessor output



#---------------------- GTK 

ifeq ($(GTK),)
$(warning GTK undefined (no config.makefile generated yet?))
else
ifeq ($(GTK),2)
	GTKCFLAGS:= $(shell pkg-config --cflags gtk+-2.0) -fPIC
	GTKLIBS:= $(shell pkg-config --libs gtk+-2.0 | $(SED) 's/-pthread//')

	cflags += $(GTKCFLAGS)
	dflags += -DARB_GTK
else
ifeq ($(GTK),3)
	GTKCFLAGS:= $(shell pkg-config --cflags gtk+-3.0) -fPIC
	GTKLIBS:= $(shell pkg-config --libs gtk+-3.0 | $(SED) 's/-pthread//')

	cflags += $(GTKCFLAGS)
	dflags += -DARB_GTK
	dflags += -DARB_GTK3
else
$(error Unsupported GTK version $(GTK) (possible values: 2, 3))
endif
endif
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

        GLEWLIB := -lGLEW -lGLw
	ifdef DARWIN
		GLUTLIB := -glut
	else
		GLUTLIB := -lglut
	endif

        GL_LIBS_SYS := $(GL_LIB_SYS) $(GL_PNGLIBS_SYS) $(GLEWLIB) $(GLUTLIB)
        GL_LIBS_ARB := $(GL_LIB_ARB) $(GL_PNGLIBS_ARB)

        RNA3D_LIB := RNA3D/RNA3D.a

else

        GL_LIBS_ARB:=# no opengl -> no libs
        GL_LIBS_SYS:=# no opengl -> no libs
        GL:=# don't build ARB openGL libs

        RNA3D_LIB :=

endif

RNA3D_LIB_4_DEPENDS := RNA3D/RNA3D.a

GL_LIBS:=$(GL_LIBS_ARB) $(GL_LIBS_SYS)

#---------------------- tiff lib:

TIFFLIBS := -ltiff

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
		cdynamic = -rdynamic -Wl,--export-dynamic
		ldynamic = --export-dynamic
	endif
endif

#---------------------- system dependent commands

ifdef DARWIN
	TIME:=gtime
else
	TIME:=/usr/bin/time
endif

# -------------------------------------------------------------------------
#	Don't put any machine/version/etc conditionals below!
#	(instead define variables above)
# -------------------------------------------------------------------------

cflags += -W -Wall $(dflags) $(extended_warnings) $(cdynamic)

cppflags := $(extended_cpp_warnings)

ifeq ('$(USE_GCC_47_OR_HIGHER)','yes')
cppflags += -std=gnu++11# see also TEMPLATES/cxxforward.h@USE_Cxx11
else
# only use for gcc versions between 4.3 and <4.7 (4.7++ adds -Wc++11-compat above)
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

LINK_STATIC_LIB := ld $(lflags) $(ldynamic) -r -o# link static lib
LINK_EXECUTABLE := $(GPP) $(lflags) $(cdynamic) -o# link executable (c++)

ifeq ($(LINK_STATIC),1)
SHARED_LIB_SUFFIX = a# static lib suffix
LINK_SHARED_LIB := $(LINK_STATIC_LIB)
else
SHARED_LIB_SUFFIX = so# shared lib suffix
LINK_SHARED_LIB := $(GPP) $(lflags) $(cdynamic) -shared $(GCOVFLAGS) -o# link shared lib
endif

# other used tools


ifdef DARWIN
	XMKMF := $(PREFIX)/bin/xmkmf
else
	XMKMF := xmkmf
endif

MAKEDEPEND_PLAIN = makedepend

MAKEDEPEND = $(FORCEMASK);$(MAKEDEPEND_PLAIN)

#SEP:=--------------------------------------------------------------------------------
SEP=[`date +%M:%S.%N`] ------------------------------------------------
# to analyse timings run
# make -j9 clean; make -j9 all  | grep '^\[' | sort
# make -j9 "TIMED_TARGET=perl" clean_timed_target | grep '^\[' | sort



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
		@echo ' show         - show available shortcuts (AKA subtargets)'
		@echo ' up           - shortcut for depends+proto+tags'
ifeq ($(UNIT_TESTS),1)
		@echo ' ut           - only run tests'
endif
		@echo ' modified     - rebuild files modified in svn checkout (does touch)'
		@echo ' touch        - touch files modified in svn checkout'
		@echo ''
		@echo 'Internal maintenance:'
		@echo ''
		@echo ' rel_minor   - build a release (increases minor version number)'
		@echo ' rel_major   - build a release (increases MAJOR version number)'
		@echo ' tarfile     - make rebuild and create arb version tarfile ("tarfile_quick" to skip rebuild)'
		@echo ' save        - save all basic ARB sources into arbsrc_DATE ("savetest" to check filelist)'
		@echo ' patch       - save svn diff to patchfile'
		@echo ' source_doc  - create doxygen documentation'
		@echo ' relocated   - rebuild partly (use when you have relocated ARBHOME)'
		@echo ' check_res   - check ressource usage'
		@echo ' dep_graph   - Build dependency graphs'
		@echo ' clean_cov   - Clean coverage results'
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
		"$(GCC)" \
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
	@touch -t 198001010000 SOURCE_TOOLS/stamp.tabBrake
	@$(MAKE) check_tabs


# ---------------------

check_setup: check_ENVIRONMENT check_DEBUG check_ARB_64 check_DEVELOPER check_GCC_VERSION 
		@echo Your setup seems to be ok.

checks: check_setup check_tabs
	@rm -f SOURCE_TOOLS/postcompile.sav


# end test section ------------------------------

CORE_LIB=-lCORE
ARBDB_LIB=-lARBDB $(CORE_LIB)
LIBS = $(ARBDB_LIB) $(SYSLIBS)

GUI_LIBS_PREFIX:=
ifdef DARWIN
#       this seem to be added at wrong place, since opengl is only needed to link EDIT4 
        GUI_LIBS_PREFIX:=-framework GLUT -framework OpenGL
endif

GUI_LIBS = $(GUI_LIBS_PREFIX) $(LIBS) -lAWT  
GUI_LIBS += -lWINDOW $(GTKLIBS)

LIBPATH = -L$(ARBHOME)/lib

DEST_LIB = lib
DEST_BIN = bin

AINCLUDES := -I. -I$(ARBHOME)/INCLUDE
CPPINCLUDES := -I. -I$(ARBHOME)/INCLUDE

MAKEDEPENDFLAGS := -- -DARB_OPENGL -DUNIT_TESTS -D__cplusplus -I. -Y$(ARBHOME)/INCLUDE

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
ARCHS_PT_SERVER_LINK = PROBE/PROBE.a# default to old ptserver
else
# PT-server only libs
ARCHS_PT_SERVER = \
	PROBE/PROBE.a
endif
endif

ifndef ARCHS_PT_SERVER_LINK
ARCHS_PT_SERVER_LINK = $(ARCHS_PT_SERVER)
endif

# ---------------------------------------
# wrap main()

use_ARB_main=$(ARBHOME)/SOURCE_TOOLS/arb_main_cpp.o
use_ARB_main_C=$(ARBHOME)/SOURCE_TOOLS/arb_main_c.o

arbmainwrapper:
	$(MAKE) -C SOURCE_TOOLS -r "cflags = $(cflags)" mainwrapper

# ---------------------------------------
# List of standard top level directories
#
# sub-makefiles have to define the targets
# - 'depends' and
# - 'clean'
#
# when adding new libs here, also add a dependency vs 'links' or 'links_non_perl' in .@DD_links_non_perl

ARCHS = \
			$(ARCHS_PT_SERVER) \
			AISC/AISC.a \
			AISC_MKPTPS/AISC_MKPTPS.a \
			ARBDB/libARBDB.a \
			CORE/libCORE.a \
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
			PERLTOOLS/PERLTOOLS.a \
			PHYLO/PHYLO.a \
			PRIMER_DESIGN/PRIMER_DESIGN.a \
			PROBE_COM/server.a \
			PROBE_DESIGN/PROBE_DESIGN.a \
			PROBE_SET/PROBE_SET.a \
			READSEQ/READSEQ.a \
			RNA3D/RNA3D.a \
			RNACMA/RNACMA.a \
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
ARCHS_MAKEBIN = AISC_MKPTPS/AISC_MKPTPS.a AISC/AISC.a

# communication libs need aisc and aisc_mkpts:

AISC/AISC.dummy: proto_tools

comtools: AISC/AISC.dummy

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

# --------------------------------------------------------------------------------
# dependencies for linking shared libs

link_core:	core
link_db:	db link_core
link_aw:	aw link_db
link_awt:	awt link_aw

#***************************************************************************************
#		Individual_Programs_Section 
#***************************************************************************************

#***********************************	arb_ntree **************************************
NTREE = bin/arb_ntree
ARCHS_NTREE = \
		NTREE/NTREE.a \
		$(ARCHS_AP_TREE) \
		ARB_GDE/ARB_GDE.a \
		AWTC/AWTC.a \
		AWTI/AWTI.a \
		CONSENSUS_TREE/CONSENSUS_TREE.a \
		GENOM/GENOM.a \
		GENOM_IMPORT/GENOM_IMPORT.a \
		MERGE/MERGE.a \
		MULTI_PROBE/MULTI_PROBE.a \
		PRIMER_DESIGN/PRIMER_DESIGN.a \
		PROBE_DESIGN/PROBE_DESIGN.a \
		SEQ_QUALITY/SEQ_QUALITY.a \
		SERVERCNTRL/SERVERCNTRL.a \
		SL/AW_NAME/AW_NAME.a \
		SL/DB_UI/DB_UI.a \
		SL/DB_SCANNER/DB_SCANNER.a \
		SL/DB_QUERY/DB_QUERY.a \
		SL/SEQIO/SEQIO.a \
		SL/GUI_ALIVIEW/GUI_ALIVIEW.a \
		SL/HELIX/HELIX.a \
		SL/REGEXPR/REGEXPR.a \
		SL/REFENTRIES/REFENTRIES.a \
		SL/NDS/NDS.a \
		SL/ITEMS/ITEMS.a \
		SL/INSDEL/INSDEL.a \
		SL/LOCATION/LOCATION.a \
		SL/MACROS/MACROS.a \
		SL/TRANSLATE/TRANSLATE.a \
		SL/TREEDISP/TREEDISP.a \
		SL/TREE_ADMIN/TREE_ADMIN.a \
		SL/TREE_READ/TREE_READ.a \
		SL/TREE_WRITE/TREE_WRITE.a \
		STAT/STAT.a \
		XML/XML.a \

$(NTREE): $(ARCHS_NTREE:.a=.dummy) link_awt
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_NTREE) $(GUI_LIBS) || ( \
		echo "$(SEP) Link $@"; \
		echo "$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_NTREE) $(ARCHS_CLIENT_PROBE) $(GUI_LIBS) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_NTREE) $(ARCHS_CLIENT_PROBE) $(GUI_LIBS) $(EXECLIBS) && \
		echo "$(SEP) Link $@ [done]"; \
		)

#***********************************	arb_edit4 **************************************
EDIT4 = bin/arb_edit4

ARCHS_EDIT4 := \
		EDIT4/EDIT4.a \
		$(ARCHS_AP_TREE) \
		ARB_GDE/ARB_GDE.a \
		AWTC/AWTC.a \
		ISLAND_HOPPING/ISLAND_HOPPING.a \
		SECEDIT/SECEDIT.a \
		SERVERCNTRL/SERVERCNTRL.a \
		SL/AW_HELIX/AW_HELIX.a \
		SL/AW_NAME/AW_NAME.a \
		SL/FAST_ALIGNER/FAST_ALIGNER.a \
		SL/ITEMS/ITEMS.a \
		SL/GUI_ALIVIEW/GUI_ALIVIEW.a \
		SL/HELIX/HELIX.a \
		SL/TRANSLATE/TRANSLATE.a \
		SL/MACROS/MACROS.a \
		STAT/STAT.a \
		XML/XML.a \

ifeq ($(OPENGL),1)
ARCHS_EDIT4 += RNA3D/RNA3D.a
endif

LIBS_EDIT4 := $(GL_LIBS)

$(EDIT4): $(ARCHS_EDIT4:.a=.dummy) link_awt 
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_EDIT4) $(GUI_LIBS) || ( \
		echo "$(SEP) Link $@"; \
		echo "$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_EDIT4) $(ARCHS_CLIENT_NAMES) $(GUI_LIBS) $(LIBS_EDIT4) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_EDIT4) $(ARCHS_CLIENT_NAMES) $(GUI_LIBS) $(LIBS_EDIT4) $(EXECLIBS) && \
		echo "$(SEP) Link $@ [done]"; \
		)

#***********************************	arb_rnacma **************************************
RNACMA = bin/arb_rnacma
ARCHS_RNACMA = \
		RNACMA/RNACMA.a \

$(RNACMA) : $(ARCHS_RNACMA:.a=.dummy) link_db
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_RNACMA) || ( \
		echo "$(SEP) Link $@"; \
		echo "$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_RNACMA) $(LIBS) $(EXECLIBS)"; \
		$(LINK_EXECUTABLE) $@ $(LIBPATH) $(ARCHS_RNACMA) $(LIBS) $(EXECLIBS) && \
		echo "$(SEP) Link $@ [done]"; \
		)

#***********************************	arb_wetc **************************************
WETC = bin/arb_wetc
ARCHS_WETC = \
		WETC/WETC.a \
		SL/HELIX/HELIX.a \
		SL/FILTER/FILTER.a \
		XML/XML.a \

$(WETC): $(ARCHS_WETC:.a=.dummy) link_awt
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_WETC) $(GUI_LIBS) || ( \
		echo "$(SEP) Link $@"; \
		echo "$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_WETC) $(GUI_LIBS) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_WETC) $(GUI_LIBS) $(EXECLIBS) && \
		echo "$(SEP) Link $@ [done]"; \
		)

#***********************************	arb_dist **************************************
DIST = bin/arb_dist
ARCHS_DIST = \
		$(ARCHS_AP_TREE) \
		CONSENSUS_TREE/CONSENSUS_TREE.a \
		DIST/DIST.a \
		EISPACK/EISPACK.a \
		SERVERCNTRL/SERVERCNTRL.a \
		SL/GUI_ALIVIEW/GUI_ALIVIEW.a \
		SL/HELIX/HELIX.a \
		SL/MATRIX/MATRIX.a \
		SL/MACROS/MACROS.a \
		SL/NDS/NDS.a \
		SL/ITEMS/ITEMS.a \
		SL/NEIGHBOURJOIN/NEIGHBOURJOIN.a \
		XML/XML.a \

$(DIST): $(ARCHS_DIST:.a=.dummy) link_awt
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_DIST) $(GUI_LIBS) || ( \
		echo "$(SEP) Link $@"; \
		echo "$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_DIST) $(ARCHS_CLIENT_PROBE) $(GUI_LIBS) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_DIST) $(ARCHS_CLIENT_PROBE) $(GUI_LIBS) $(EXECLIBS) && \
		echo "$(SEP) Link $@ [done]"; \
		)

#***********************************	arb_pars **************************************
PARSIMONY = bin/arb_pars
ARCHS_PARSIMONY = \
		$(ARCHS_AP_TREE) \
		PARSIMONY/PARSIMONY.a \
		SERVERCNTRL/SERVERCNTRL.a \
		SL/AW_NAME/AW_NAME.a \
		SL/GUI_ALIVIEW/GUI_ALIVIEW.a \
		SL/HELIX/HELIX.a \
		SL/MACROS/MACROS.a \
		SL/NDS/NDS.a \
		SL/ITEMS/ITEMS.a \
		SL/TRANSLATE/TRANSLATE.a \
		SL/TREEDISP/TREEDISP.a \
		XML/XML.a \

$(PARSIMONY): $(ARCHS_PARSIMONY:.a=.dummy) link_awt
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_PARSIMONY) $(GUI_LIBS) || ( \
		echo "$(SEP) Link $@"; \
		echo "$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_PARSIMONY) $(ARCHS_CLIENT_NAMES) $(GUI_LIBS) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_PARSIMONY) $(ARCHS_CLIENT_NAMES) $(GUI_LIBS) $(EXECLIBS) && \
		echo "$(SEP) Link $@ [done]"; \
		)

#*********************************** arb_convert_aln **************************************
CONVERT_ALN = bin/arb_convert_aln
ARCHS_CONVERT_ALN =	\
		CONVERTALN/CONVERTALN.a \

$(CONVERT_ALN) : $(ARCHS_CONVERT_ALN:.a=.dummy) link_db
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_CONVERT_ALN) || ( \
		echo "$(SEP) Link $@"; \
		echo "$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_CONVERT_ALN) $(LIBS) $(EXECLIBS)"; \
		$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_CONVERT_ALN) $(LIBS) $(EXECLIBS) && \
		echo "$(SEP) Link $@ [done]"; \
		)

#*********************************** arb_treegen **************************************

TREEGEN = bin/arb_treegen
ARCHS_TREEGEN =	\
		TREEGEN/TREEGEN.a \

$(TREEGEN) :  $(ARCHS_TREEGEN:.a=.dummy)
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_TREEGEN) || ( \
		echo "$(SEP) Link $@"; \
		echo "$(LINK_EXECUTABLE) $@ $(use_ARB_main_C) $(LIBPATH) $(ARCHS_TREEGEN) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(use_ARB_main_C) $(LIBPATH) $(ARCHS_TREEGEN) $(EXECLIBS) && \
		echo "$(SEP) Link $@ [done]"; \
		)

#***********************************	arb_naligner **************************************
NALIGNER = bin/arb_naligner
ARCHS_NALIGNER = \
		NALIGNER/NALIGNER.a \
		SERVERCNTRL/SERVERCNTRL.a \
		SL/HELIX/HELIX.a \

$(NALIGNER): $(ARCHS_NALIGNER:.a=.dummy) link_db
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_NALIGNER) || ( \
		echo "$(SEP) Link $@"; \
		echo "$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_NALIGNER) $(ARCHS_CLIENT_PROBE) $(LIBS) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_NALIGNER) $(ARCHS_CLIENT_PROBE) $(LIBS) $(EXECLIBS) && \
		echo "$(SEP) Link $@ [done]"; \
		)

#***********************************	arb_phylo **************************************
PHYLO = bin/arb_phylo
ARCHS_PHYLO = \
		PHYLO/PHYLO.a \
		SL/HELIX/HELIX.a \
		SL/FILTER/FILTER.a \
		SL/MATRIX/MATRIX.a \
		SL/MACROS/MACROS.a \
		XML/XML.a \

$(PHYLO): $(ARCHS_PHYLO:.a=.dummy) link_awt
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_PHYLO) $(GUI_LIBS) || ( \
		echo "$(SEP) Link $@"; \
		echo "$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_PHYLO) $(GUI_LIBS) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_PHYLO) $(GUI_LIBS) $(EXECLIBS) && \
		echo "$(SEP) Link $@ [done]"; \
		)

#***************************************************************************************
#					SERVER SECTION
#***************************************************************************************

#***********************************	arb_db_server **************************************
DBSERVER = bin/arb_db_server
ARCHS_DBSERVER = \
		DBSERVER/DBSERVER.a \
		SERVERCNTRL/SERVERCNTRL.a \

$(DBSERVER): $(ARCHS_DBSERVER:.a=.dummy) link_db
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_DBSERVER) $(ARBDB_LIB) || ( \
		echo "$(SEP) Link $@"; \
		echo "$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_DBSERVER) $(ARBDB_LIB) PROBE_COM/client.a $(SYSLIBS) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_DBSERVER) $(ARBDB_LIB) PROBE_COM/client.a $(SYSLIBS) $(EXECLIBS) && \
		echo "$(SEP) Link $@ [done]"; \
		)

#***********************************	arb_pt_server **************************************
PROBE = bin/arb_pt_server
ARCHS_PROBE_COMMON = \
		SERVERCNTRL/SERVERCNTRL.a \
		SL/HELIX/HELIX.a \
		SL/PTCLEAN/PTCLEAN.a \

ARCHS_PROBE_LINK = \
		$(ARCHS_PROBE_COMMON) \
		$(ARCHS_PT_SERVER_LINK) \

ARCHS_PROBE_DEPEND = \
		$(ARCHS_PROBE_COMMON) \
		$(ARCHS_PT_SERVER) \

$(PROBE): $(ARCHS_PROBE_DEPEND:.a=.dummy) link_db 
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_PROBE_LINK) $(ARBDB_LIB) $(ARCHS_CLIENT_PROBE) config.makefile || ( \
		echo "$(SEP) Link $@"; \
		echo "$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_PROBE_LINK) $(ARBDB_LIB) $(ARCHS_CLIENT_PROBE) PROBE_COM/server.a $(SYSLIBS) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_PROBE_LINK) $(ARBDB_LIB) $(ARCHS_CLIENT_PROBE) PROBE_COM/server.a $(SYSLIBS) $(EXECLIBS) && \
		echo "$(SEP) Link $@ [done]"; \
		)

#***********************************	arb_name_server **************************************
NAMES = bin/arb_name_server
ARCHS_NAMES = \
		NAMES/NAMES.a \
		SERVERCNTRL/SERVERCNTRL.a \

$(NAMES): $(ARCHS_NAMES:.a=.dummy) link_db
	@SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_NAMES) $(ARBDB_LIB) $(ARCHS_CLIENT_NAMES) || ( \
		echo "$(SEP) Link $@"; \
		echo "$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_NAMES) $(ARBDB_LIB) $(ARCHS_CLIENT_NAMES) NAMES_COM/server.a $(SYSLIBS) $(EXECLIBS)" ; \
		$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(LIBPATH) $(ARCHS_NAMES) $(ARBDB_LIB) $(ARCHS_CLIENT_NAMES) NAMES_COM/server.a $(SYSLIBS) $(EXECLIBS) && \
		echo "$(SEP) Link $@ [done]"; \
		)

#***********************************	SHARED LIBRARIES SECTION  **************************************

prepare_libdir: addlibs

addlibs:
	(perl $(ARBHOME)/SOURCE_TOOLS/provide_libs.pl \
				"arbhome=$(ARBHOME)" \
				"opengl=$(OPENGL)" \
				"link_static=$(LINK_STATIC)" \
	)

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
		depends
	@grep "^# DO NOT DELETE" $(@D)/Makefile >/dev/null
	@cat $(@D)/Makefile \
		| SOURCE_TOOLS/fix_depends.pl "(from main)" \
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


# stop wrong targets
target_is_missing_lib_prefix:
	@echo "Error: Denied to build shared library target with missing 'lib'-prefix"
	false

ARBDB/ARBDB.dummy:           target_is_missing_lib_prefix
CORE/CORE.dummy:             target_is_missing_lib_prefix
AWT/AWT.dummy:               target_is_missing_lib_prefix
WINDOW/WINDOW.dummy: target_is_missing_lib_prefix

# rule to generate main target (normally a library):
%.dummy:
	@( export ID=$$$$; LANG=C; \
	(( \
	    echo "$(SEP) Make $(@D)"; \
	    $(MAKE) -C $(@D) -r \
		"AUTODEPENDS=1" \
		"MAIN = $(@F:.dummy=.a)" \
		"cflags = $(cflags) -DIN_ARB_$(subst /,_,$(@D))" && \
	    echo "$(SEP) Make $(@D) [done]"; \
	) >$(@D).$$ID.log 2>&1 && (cat $(@D).$$ID.log;rm $(@D).$$ID.log)) || (cat $(@D).$$ID.log;rm $(@D).$$ID.log;false))

# Additional dependencies for subtargets:

PROBE_COM/PROBE_COM.dummy : comtools
NAMES_COM/NAMES_COM.dummy : comtools

com: PROBE_COM/PROBE_COM.dummy NAMES_COM/NAMES_COM.dummy

PROBE_COM/server.dummy:
	@echo Unwanted request to make target $<
	false

PROBE_COM/client.dummy:
	@echo Unwanted request to make target $<
	false

NAMES_COM/server.dummy: 
	@echo Unwanted request to make target $<
	false

NAMES_COM/client.dummy:
	@echo Unwanted request to make target $<
	false


ARBDB/libARBDB.dummy:			links
CORE/libCORE.dummy:			links

PERLTOOLS/PERLTOOLS.dummy:		core db

# all subdirs perl not depends on go here (ADD_links_non_perl)
AWT/libAWT.dummy:			links_non_perl
AWTI/AWTI.dummy:			links_non_perl
CONSENSUS_TREE/CONSENSUS_TREE.dummy:	links_non_perl
CONVERTALN/CONVERTALN.dummy:		links_non_perl
DBSERVER/DBSERVER.dummy:		links_non_perl
DIST/DIST.dummy:			links_non_perl
EDIT4/EDIT4.dummy:			links_non_perl genheaders com
EISPACK/EISPACK.dummy:			links_non_perl
GDE/GDE.dummy:				links_non_perl
GENOM/GENOM.dummy:			links_non_perl
GENOM_IMPORT/GENOM_IMPORT.dummy:	links_non_perl
ISLAND_HOPPING/ISLAND_HOPPING.dummy:	links_non_perl
MERGE/MERGE.dummy:			links_non_perl
NTREE/NTREE.dummy:			links_non_perl genheaders
PARSIMONY/PARSIMONY.dummy:		links_non_perl
PHYLO/PHYLO.dummy:			links_non_perl
PRIMER_DESIGN/PRIMER_DESIGN.dummy:	links_non_perl
PROBE_SET/PROBE_SET.dummy:		links_non_perl link_db
READSEQ/READSEQ.dummy:			links_non_perl
RNACMA/RNACMA.dummy:			links_non_perl header_libs
SECEDIT/SECEDIT.dummy:			links_non_perl
SEQ_QUALITY/SEQ_QUALITY.dummy:		links_non_perl
SERVERCNTRL/SERVERCNTRL.dummy:		links_non_perl com
SL/ALIVIEW/ALIVIEW.dummy:		links_non_perl
SL/AP_TREE/AP_TREE.dummy:		links_non_perl
SL/ARB_TREE/ARB_TREE.dummy:		links_non_perl
SL/AW_HELIX/AW_HELIX.dummy:		links_non_perl
SL/DB_QUERY/DB_QUERY.dummy:		links_non_perl
SL/DB_SCANNER/DB_SCANNER.dummy:		links_non_perl
SL/DB_UI/DB_UI.dummy:			links_non_perl
SL/FAST_ALIGNER/FAST_ALIGNER.dummy:	links_non_perl
SL/FILTER/FILTER.dummy:			links_non_perl
SL/GUI_ALIVIEW/GUI_ALIVIEW.dummy:	links_non_perl
SL/HELIX/HELIX.dummy:			links_non_perl
SL/INSDEL/INSDEL.dummy:			links_non_perl
SL/ITEMS/ITEMS.dummy:			links_non_perl
SL/LOCATION/LOCATION.dummy:		links_non_perl
SL/MACROS/MACROS.dummy:			links_non_perl
SL/MATRIX/MATRIX.dummy:			links_non_perl
SL/NDS/NDS.dummy:			links_non_perl
SL/NEIGHBOURJOIN/NEIGHBOURJOIN.dummy:	links_non_perl
SL/PRONUC/PRONUC.dummy:			links_non_perl
SL/PTCLEAN/PTCLEAN.dummy:		links_non_perl link_db
SL/REFENTRIES/REFENTRIES.dummy:		links_non_perl
SL/REGEXPR/REGEXPR.dummy:		links_non_perl
SL/SEQIO/SEQIO.dummy:			links_non_perl
SL/SEQUENCE/SEQUENCE.dummy:		links_non_perl
SL/TRANSLATE/TRANSLATE.dummy:		links_non_perl
SL/TREE_ADMIN/TREE_ADMIN.dummy:		links_non_perl
SL/TREE_READ/TREE_READ.dummy:		links_non_perl
SL/TREE_WRITE/TREE_WRITE.dummy:		links_non_perl
SL/TREEDISP/TREEDISP.dummy:		links_non_perl
STAT/STAT.dummy:			links_non_perl
TREEGEN/TREEGEN.dummy:			links_non_perl
WETC/WETC.dummy:			links_non_perl
WINDOW/libWINDOW.dummy:		links_non_perl
XML/XML.dummy:				links_non_perl

ifeq ($(OPENGL),1)
GL/glAW/glAW.dummy: links_non_perl
GL/glpng/glpng.dummy: links_non_perl
GL/GL.dummy: GL/glAW/glAW.dummy GL/glpng/glpng.dummy
RNA3D/RNA3D.dummy: links_non_perl gl
endif

UNIT_TESTER/UNIT_TESTER.dummy:		link_db \
	SERVERCNTRL/SERVERCNTRL.dummy \

TOOLS/TOOLS.dummy : links_non_perl link_db \
	SERVERCNTRL/SERVERCNTRL.dummy \
	SL/TREE_WRITE/TREE_WRITE.dummy \
	SL/TREE_READ/TREE_READ.dummy \
	CONSENSUS_TREE/CONSENSUS_TREE.dummy \
	XML/XML.dummy \

AWTC/AWTC.dummy :   			com

NAMES/NAMES.dummy : 			com
SL/AW_NAME/AW_NAME.dummy : 		com

PROBE/PROBE.dummy : 			com
ptpan/PROBE.dummy : 			com
MULTI_PROBE/MULTI_PROBE.dummy : 	com
PROBE_DESIGN/PROBE_DESIGN.dummy : 	com
NALIGNER/NALIGNER.dummy : 		com

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
		@echo '  aw     GUI lib (GTK)'
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

dep_graph:
	@echo "Building some dependency graphs"
	SOURCE_TOOLS/dependency_graphs.pl

help:   HELP_SOURCE/HELP_SOURCE.dummy

# @@@ when backtracing code is in libCORE, link vs ARBDB is no longer needed 
HELP_SOURCE/HELP_SOURCE.dummy: link_db xml menus

db:	ARBDB/libARBDB.dummy
core:	CORE/libCORE.dummy
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

di:	$(DIST)
ph:	$(PHYLO)
pa:	$(PARSIMONY)
tg:	$(TREEGEN)

ifeq ($(OPENGL),1)
3d:	RNA3D/RNA3D.dummy
gl:	GL/GL.dummy
else
noopengl:
	@echo "invalid target for OPENGL=0"
3d: noopengl
gl: noopengl
endif

SL/SL.dummy: com

ds:	$(DBSERVER)
pt:	$(PROBE)
pst: 	PROBE_SET/PROBE_SET.dummy
pd:	PROBE_DESIGN/PROBE_DESIGN.dummy
na:	$(NAMES)
sq:	SEQ_QUALITY/SEQ_QUALITY.dummy
cma:    $(RNACMA)

sec:	SECEDIT/SECEDIT.dummy

e4:	$(EDIT4) readseq menus

gi:	GENOM_IMPORT/GENOM_IMPORT.dummy
wetc:	$(WETC)

xml:	XML/XML.dummy
xmlin:  XML_IMPORT/XML_IMPORT.dummy# broken
stat:   STAT/STAT.dummy $(NTREE) $(EDIT4)
fa:	SL/FAST_ALIGNER/FAST_ALIGNER.dummy

#********************************************************************************

up_by_remake: depends proto

up: up_by_remake tags valgrind_update

#********************************************************************************

touch:
	SOURCE_TOOLS/touch_modified.pl

modified: touch
	$(MAKE) all

#********************************************************************************

libdepends:
	$(MAKE) -C "SOURCE_TOOLS" \
		"RNA3D_LIB=$(RNA3D_LIB_4_DEPENDS)" \
		libdepends

#********************************************************************************

# create generated headers:
genheaders: TEMPLATES/TEMPLATES.dummy

clrdotdepends:
	-rm PROBE_COM/.depends
	-rm NAMES_COM/.depends
	-rm PERL2ARB/.depends

comdepends: comtools clrdotdepends
	@echo "$(SEP) Partially build com interface"
	$(MAKE) PROBE_COM/PROBE_COM.depends NAMES_COM/NAMES_COM.depends
	$(MAKE) PROBE_COM/server.depends    NAMES_COM/server.depends

depends: genheaders comdepends
	@echo "$(SEP) Updating other dependencies"
	$(MAKE) $(subst NAMES_COM/server.depends,,$(subst PROBE_COM/server.depends,,$(ARCHS:.a=.depends))) \
		HELP_SOURCE/HELP_SOURCE.depends
	$(MAKE) libdepends

depend: depends

# ------------------------------------------------------------
# dependency generation tests for AISC code
#(all should work w/o creating wrong dependencies;
# neither in XXX_COM/Makefile nor in code using AISC interface)
dependstest1: silent_clean
	$(MAKE) depends
dependstest2: silent_clean
	$(MAKE) com
dependstest3: silent_clean
	$(MAKE) aw
dependstest4: silent_clean
	$(MAKE) pt
dependstest5: silent_clean
	$(MAKE) na
dependstest6: silent_clean
	$(MAKE) nt
dependstest7: silent_clean
	$(MAKE) all
# ------------------------------------------------------------

AISC_MKPTPS/AISC_MKPTPS.dummy: links

proto_tools: AISC_MKPTPS/AISC_MKPTPS.dummy

proto: proto_tools
	@echo $(SEP) Updating prototypes
	$(MAKE) \
		ARBDB/ARBDB.proto \
		AISC_COM/AISC_COM.proto \
		ARB_GDE/ARB_GDE.proto \
		CORE/CORE.proto \
		CONVERTALN/CONVERTALN.proto \
		NTREE/NTREE.proto \
		MERGE/MERGE.proto \
		$(ARCHS_PT_SERVER:.a=.proto) \
		SERVERCNTRL/SERVERCNTRL.proto \
		SL/SL.proto \

#********************************************************************************

valgrind_update: links
	@echo $(SEP) Updating for valgrind
	$(MAKE) -C SOURCE_TOOLS valgrind_update

#********************************************************************************

TAGFILE=TAGS
TAGFILE_TMP=TAGS.tmp

TAG_SOURCE_HEADERS=TAGS.headers
TAG_SOURCE_CODE=TAGS.codefiles

TAG_SOURCE_LISTS=$(TAG_SOURCE_HEADERS) $(TAG_SOURCE_CODE)

ETAGS=ctags -e -f $(TAGFILE_TMP) --sort=no --if0=no --extra=q
ETAGS_TYPES=--C-kinds=cgnsut --C++-kinds=cgnsut
ETAGS_FUN  =--C-kinds=fm     --C++-kinds=fm
ETAGS_REST =--C-kinds=dev    --C++-kinds=dev

$(TAG_SOURCE_HEADERS): links
#	find . \( -name '*.[ch]xx' -o -name "*.[ch]" \) -type f | grep -v -i perl5 > $@
#	workaround a bug in ctags 5.8:
	find . \( -name '*.hxx' -o -name "*.h" \) -type f | grep -v -i perl5 | $(SED) -e 's/^.\///g' > $@

$(TAG_SOURCE_CODE): links
#	find . \( -name '*.[ch]xx' -o -name "*.[ch]" \) -type f | grep -v -i perl5 > $@
#	workaround a bug in ctags 5.8:
	find . \( -name '*.cxx' -o -name "*.c" \) -type f | grep -v -i perl5 | $(SED) -e 's/^.\///g' > $@

tags: $(TAG_SOURCE_LISTS)
	$(ETAGS)    $(ETAGS_TYPES) -L $(TAG_SOURCE_HEADERS)
	$(ETAGS) -a $(ETAGS_FUN)   -L $(TAG_SOURCE_HEADERS)
	$(ETAGS) -a $(ETAGS_REST)  -L $(TAG_SOURCE_HEADERS)
	$(ETAGS) -a $(ETAGS_TYPES) -L $(TAG_SOURCE_CODE)
	$(ETAGS) -a $(ETAGS_FUN)   -L $(TAG_SOURCE_CODE)
	$(ETAGS) -a $(ETAGS_REST)  -L $(TAG_SOURCE_CODE)
	mv_if_diff $(TAGFILE_TMP) $(TAGFILE)
	rm $(TAG_SOURCE_LISTS)

#********************************************************************************

LINKSTAMP=SOURCE_TOOLS/stamp.generate_all_links

links: checks $(LINKSTAMP) arbmainwrapper
links_no_checks: $(LINKSTAMP) arbmainwrapper

forcelinks:
	-rm $(LINKSTAMP)
	$(MAKE) links

$(LINKSTAMP): SOURCE_TOOLS/generate_all_links.sh
	+SOURCE_TOOLS/generate_all_links.sh
	touch $(LINKSTAMP)

clean_links:
#       avoid to delete linked pts, nas or arb_tcp.dat:
	find . -path './lib' -prune -o -type l -exec rm {} \;
	@rm -f $(LINKSTAMP) lib/inputMasks/format.readme

redo_links: clean_links
	$(MAKE) links_no_checks

#********************************************************************************

header_libs:
	$(MAKE) -C HEADERLIBS all

#********************************************************************************

gde:		GDE/GDE.dummy
GDE:		gde
agde: 		ARB_GDE/ARB_GDE.dummy

tools:		TOOLS/TOOLS.dummy

convert:	$(CONVERT_ALN)
readseq:	READSEQ/READSEQ.dummy

#***************************************************************************************
#			Some user commands
#***************************************************************************************

menus: binlink links
	@(( \
		echo "$(SEP) Make GDEHELP"; \
		$(MAKE) -C GDEHELP -r all && \
		echo "$(SEP) Make GDEHELP [done]"; \
	) > GDEHELP.log 2>&1 && (cat GDEHELP.log;rm GDEHELP.log)) || (cat GDEHELP.log;rm GDEHELP.log;false)

ifeq ($(DEBUG),1)
BIN_TARGET=develall
else
BIN_TARGET=all
endif


binlink:
	$(MAKE) -C bin $(BIN_TARGET)

preplib:
	(cd lib;$(MAKE) all)

# --------------------------------------------------------------------------------
# This section is quite tricky:
#
# make 'perl' is a BIG target, so when it has to be made, it has to be started
# as early as possible to reduce overall compile time. Since 'make' does not
# provide any priotities, i force it to build all 'perl'-prerequisites early, by
# adding  artificial dependencies to these prerequisites 
#
# That behavior is likely to be system-dependent.
# My goal was only to make it work on my current development system,
# where this saves about 20% of overall build time.

ifeq ($(WITHPERL),1)
links_non_perl:	PERLTOOLS/PERLTOOLS.dummy
perltools:	links_non_perl
perl:		realperl
else
links_non_perl:	links
perl:
	$(MAKE) "WITHPERL=1" perl
endif

# ---------------------------------------- perl

ifeq ($(ARB_64),$(BUILDHOST_64))
TEST_PERL_SCRIPTS=$(MAKE) -C PERL_SCRIPTS/test test
else
TEST_PERL_SCRIPTS=echo "skipping perl script compilation (when cross-compiling)"
endif

realperl: perltools
	(( \
		echo "$(SEP) Make PERL2ARB" ; \
		$(TIME) $(MAKE) -C PERL2ARB -r -f Makefile.main \
			"AUTODEPENDS=1" \
			"dflags=$(dflags)" \
			"cross_cflags=$(cross_cflags) $(dflags)" \
			"cross_lflags=$(cross_lflags)" \
			all && \
		$(TEST_PERL_SCRIPTS) && \
		echo "$(SEP) Make PERL2ARB [done]" ; \
	) > PERL2ARB.log 2>&1 && (cat PERL2ARB.log;rm PERL2ARB.log)) || (cat PERL2ARB.log;rm PERL2ARB.log;false)

perl_clean:
	@$(MAKE) -C PERL2ARB -r -f Makefile.main \
		"AUTODEPENDS=0" \
		clean

PERL2ARB/PERL2ARB.clean:
	$(MAKE) perl_clean

# ---------------------------------------- bindings to script languages

bindings: lib/libCORE.so lib/libARBDB.so
	$(MAKE) -C BINDINGS all ARBDEPENDS="$(^:%=../%)" DFLAGS="$(dflags)"

bindings_clean:
	$(MAKE) -C BINDINGS clean

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
	@find . \(	-name '*%' \
			-o -name '*.bak' \
			-o -name '*~' \) \
			-o -name 'core' \
	        -exec rm -v {} \;

bin_reinit:
	$(MAKE) bin/bin.clean
	$(MAKE) -C "bin" all

clean_directories:
	-rm -rf \
		$(ARBHOME)/PROBE_SET/bin \
		$(ARBHOME)/INCLUDE \

libclean:
	-find $(ARBHOME) -type f \( -name '*.a' ! -type l \) -exec rm -f {} \;

objclean:
	-find $(ARBHOME) -type f \( -name '*.o' ! -type l \) -exec rm -f {} \;

# bin.clean and HELP_SOURCE.clean interfere
clean3:
	$(MAKE) bin/bin.clean
	$(MAKE) HELP_SOURCE/HELP_SOURCE.clean

clean2: $(ARCHS:.a=.clean) \
		clean3 \
		rmbak \
		libclean \
		objclean \
		lib/lib.clean \
		GDEHELP/GDEHELP.clean \
		HEADERLIBS/HEADERLIBS.clean \
		SOURCE_TOOLS/SOURCE_TOOLS.clean \
		UNIT_TESTER/UNIT_TESTER.clean \
		TEMPLATES/TEMPLATES.clean \
		perl_clean \
		clean_directories \

	rm -f *.last_gcc config.makefile.bak

# links are needed for cleanup
clean: redo_links
	$(MAKE) clean2
	$(MAKE) clean_cov_all clean_links

silent_clean:
	$(MAKE) clean >/dev/null

# 'relocated' is about 50% faster than 'rebuild'
reloc_clean: links
	@echo "---------------------------------------- Relocation cleanup"
	$(MAKE) \
		perl_clean \
		GDEHELP/GDEHELP.clean \
		HELP_SOURCE/genhelp/genhelp.clean \
		bin/bin.clean \
		libclean \
		objclean

relocated: links
	$(MAKE) reloc_clean
	@echo "---------------------------------------- and remake"
	$(MAKE) build

# -----------------------------------
# some stress tests:

rebuild4ever: rebuild
	$(MAKE) rebuild4ever

build4ever: build
	$(MAKE) build4ever

clean4ever: clean
	$(MAKE) clean4ever

test4ever: ut
	$(MAKE) test4ever

perl4ever: clean 
	$(MAKE) links
	$(MAKE) perl
	$(MAKE) perl4ever


# -----------------------------------

rebuild: clean
	$(MAKE) all

relink: bin/bin.clean libclean
	$(MAKE) build

tarfile: rebuild
	$(MAKE) prepare_libdir 
	util/arb_compress

tarfile_quick: build
	$(MAKE) prepare_libdir 
	util/arb_compress

save: sourcetarfile 

patch:
	SOURCE_TOOLS/arb_create_patch.sh arbPatch

# test early whether save will work
savetest:
	@util/arb_srclst.pl >/dev/null

testsave: savetest

sourcetarfile: rmbak
	util/arb_save

save2: rmbak
	util/arb_save ignore

save_test: rmbak
	@echo "Testing source list.."
	@util/arb_srclst.pl > /dev/null

save_test_no_error:
	@-$(MAKE) save_test

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
	$(MAKE) genheaders # auto upgrades version early
	$(MAKE) tarfile 
	$(MAKE) sourcetarfile

release_quick:
	-rm arb.tgz arbsrc.tgz
	$(MAKE) tarfile_quick
	$(MAKE) sourcetarfile

# --------------------------------------------------------------------------------

MAKE_IF_COMMITTED=$(MAKE) -C SOURCE_TOOLS -f Makefile.commitbuild

build_CTARGET:
	+$(MAKE_IF_COMMITTED) "CTARGET=$(CTARGET)" build_CTARGET

reset_committed_build:
	+$(MAKE_IF_COMMITTED) reset

# --------------------------------------------------------------------------------

arbapplications: nt pa e4 wetc pt na nal di ph ds wetc cma

arb_external: convert tools gde readseq tg pst xmlin

arb_no_perl: arbapplications help arb_external

arb:
	$(MAKE) "WITHPERL=1" perl arb_no_perl

# --------------------------------------------------------------------------------
# special targets for SOURCE_TOOLS/remake_after_change.pl

rac_arb_dist:		di
rac_arb_edit4:		e4
rac_arb_ntree:		nt
rac_arb_pars:		pa
rac_arb_phylo:		ph
rac_arb_wetc:		wetc
rac_arb_naligner:	nal
rac_arb_pt_server:	pt
rac_arb_db_server:	ds
rac_arb_name_server:	na
rac_arb_convert_aln:	convert
rac_arb_treegen:	tg
rac_arb_rnacma:		cma

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
	$(RNA3D_TEST) \
	ARB_GDE/ARB_GDE.test \
	AWTI/AWTI.test \
	DIST/DIST.test \
	EISPACK/EISPACK.test \
	GENOM/GENOM.test \
	GL/glAW/libglAW.test \
	GL/glpng/libglpng_arb.test \
	ISLAND_HOPPING/ISLAND_HOPPING.test \
	NALIGNER/NALIGNER.test \
	NAMES/NAMES.test \
	PARSIMONY/PARSIMONY.test \
	PHYLO/PHYLO.test \
	PRIMER_DESIGN/PRIMER_DESIGN.test \
	PROBE_DESIGN/PROBE_DESIGN.test \
	SECEDIT/SECEDIT.test \
	SEQ_QUALITY/SEQ_QUALITY.test \
	SERVERCNTRL/SERVERCNTRL.test \
	SL/ALIVIEW/ALIVIEW.test \
	SL/AP_TREE/AP_TREE.test \
	SL/ARB_TREE/ARB_TREE.test \
	SL/AW_HELIX/AW_HELIX.test \
	SL/AW_NAME/AW_NAME.test \
	SL/DB_SCANNER/DB_SCANNER.test \
	SL/GUI_ALIVIEW/GUI_ALIVIEW.test \
	SL/HELIX/HELIX.test \
	SL/ITEMS/ITEMS.test \
	SL/LOCATION/LOCATION.test \
	SL/MATRIX/MATRIX.test \
	SL/NDS/NDS.test \
	SL/NEIGHBOURJOIN/NEIGHBOURJOIN.test \
	SL/REFENTRIES/REFENTRIES.test \
	SL/SEQUENCE/SEQUENCE.test \
	SL/TRANSLATE/TRANSLATE.test \
	SL/TREE_ADMIN/TREE_ADMIN.test \
	SL/TREE_WRITE/TREE_WRITE.test \
	STAT/STAT.test \
	TREEGEN/TREEGEN.test \
	WETC/WETC.test \
	XML/XML.test \

# untestable units

UNITS_NEED_FIX = \
	ptpan/PROBE.test \

UNITS_UNTESTABLE_ATM = \
	PROBE_SET/PROBE_SET.test \
	XML_IMPORT/XML_IMPORT.test \

# for the moment, put all units containing tests into UNITS_TESTED or UNITS_TESTED_FIRST

UNITS_TESTED_FIRST = \
	GENOM_IMPORT/GENOM_IMPORT.test \
	SL/MACROS/MACROS.test \
	SL/REGEXPR/REGEXPR.test \
	SL/FILTER/FILTER.test \
	ARBDB/libARBDB.test \
	CONSENSUS_TREE/CONSENSUS_TREE.test \
	TOOLS/arb_consensus_tree.test \
	TOOLS/arb_test.test \
	TOOLS/arb_probe.test \
	PERLTOOLS/arb_proto_2_xsub.test \
	AWTC/AWTC.test \

# plain test-libaries not linked anywhere
TEST_SANDBOXES = \
	SL/CB/CB.test \

UNITS_TESTED = \
	SL/TREE_READ/TREE_READ.test \
	DBSERVER/DBSERVER.test \
	AWT/libAWT.test \
	CORE/libCORE.test \
	SL/INSDEL/INSDEL.test \
	SL/TREEDISP/TREEDISP.test \
	NTREE/NTREE.test \
	AISC_MKPTPS/mkptypes.test \
	EDIT4/EDIT4.test \
	MERGE/MERGE.test \
	MULTI_PROBE/MULTI_PROBE.test \
	SERVERCNTRL/SERVERCNTRL.test \
	SL/FAST_ALIGNER/FAST_ALIGNER.test \
	SL/PRONUC/PRONUC.test \
	WINDOW/libWINDOW.test \
	HELP_SOURCE/arb_help2xml.test \
	CONVERTALN/CONVERTALN.test \
	SL/SEQIO/SEQIO.test \
	SL/PTCLEAN/PTCLEAN.test \
	PROBE/PROBE.test \
	SL/DB_QUERY/DB_QUERY.test \

TESTED_UNITS_MANUAL = \
	$(TEST_SANDBOXES) \
	$(UNITS_TESTED_FIRST) \
	$(UNITS_TESTED) \

#	$(UNITS_WORKING)

# see UNIT_TESTER/sym2testcode.pl@disableErrorOnUnitsWithoutTests

#TESTED_UNITS = $(TESTED_UNITS_AUTO)
TESTED_UNITS = $(TESTED_UNITS_MANUAL)

# ----------------------------------------

TEST_LOG_DIR = UNIT_TESTER/logs
TEST_RUN_SUITE=$(MAKE) $(NODIR) -C UNIT_TESTER -f Makefile.suite -r
TEST_MAKE_FLAGS=
TEST_POST_CLEAN=
ifeq ($(COVERAGE),1)
TEST_POST_CLEAN=$(MAKE) clean_cov
TEST_MAKE_FLAGS+=-j1
endif


%.test:
	-@( export ID=$$$$; mkdir -p $(TEST_LOG_DIR); \
	( \
	    echo "fake[1]: Entering directory \`$(ARBHOME)/UNIT_TESTER'"; \
	    $(MAKE) -C UNIT_TESTER -f Makefile.test -r \
		"UNITDIR=$(@D)" \
		"UNITLIBNAME=$(@F:.test=)" \
		"COVERAGE=$(COVERAGE)" \
		"cflags=$(cflags)" \
		"ARB_PID=$(ARB_PID)_$(@F)" \
		runtest; \
	    echo "fake[1]: Leaving directory \`$(ARBHOME)/UNIT_TESTER'"; \
	    $(TEST_POST_CLEAN) \
	) >$(TEST_LOG_DIR)/$(@F).log 2>&1; echo "- $(@F)")


test_base: $(UNIT_TESTER_LIB:.a=.dummy) $(subst .test,.dummy,$(TEST_SANDBOXES))

clean_cov:
	find . \( -name "*.gcda" -o -name "*.gcov" -o -name "*.cov" \) -exec rm {} \;

clean_cov_all: clean_cov
	find . \( -name "*.gcno" \) -exec rm {} \;

run_tests: test_base clean_cov
	$(MAKE) "ARB_PID=UT_$$$$" run_tests_faked_arbpid

run_tests_faked_arbpid:
	+@$(TEST_RUN_SUITE) init
	@echo "fake[2]: Entering directory \`$(ARBHOME)/UNIT_TESTER'"
	$(MAKE) $(TEST_MAKE_FLAGS) $(NODIR) $(TESTED_UNITS)
	@echo "fake[2]: Leaving directory \`$(ARBHOME)/UNIT_TESTER'"
	+@$(TEST_RUN_SUITE) cleanup
	@$(MAKE) clean_cov >/dev/null

ut:
ifeq ($(UNIT_TESTS),1)
	@echo $(MAKE) run_tests
	@$(MAKE) run_tests
else
	@echo "Not compiled with unit tests"
endif


aut:
	+@$(TEST_RUN_SUITE) unskip
	$(MAKE) ut

# --------------------------------------------------------------------------------

TIMELOG=$(ARBHOME)/arb_time.log
TIMEARGS=--append --output=$(TIMELOG) --format=" %E(%S+%U) %P [%C]"
TIMECMD=$(TIME) $(TIMEARGS)

time_one:
ifeq ($(ONE_TIMED_TARGET),)
	@echo "Error: You have to pass ONE_TIMED_TARGET to $@"
	false
else
	@echo "$(SEP) $(MAKE) $(ONE_TIMED_TARGET)"
	@$(TIMECMD) $(MAKE) $(ONE_TIMED_TARGET)
	@echo "$(SEP) $(MAKE) $(ONE_TIMED_TARGET) [done]"
endif

timed_target:
ifeq ($(TIMED_TARGET),)
	@echo "Error: You have to pass TIMED_TARGET to $@"
	false
else
	@echo "Build time:" > $(TIMELOG)
	$(MAKE) "ONE_TIMED_TARGET=$(TIMED_TARGET)" time_one
	@cat $(TIMELOG)
	@rm $(TIMELOG)
endif

timed_target_tested:
ifeq ($(TIMED_TARGET),)
	@echo "Error: You have to pass TIMED_TARGET to $@"
	false
else
	@echo "Build time:" > $(TIMELOG)
	$(MAKE) "ONE_TIMED_TARGET=$(TIMED_TARGET)" time_one
	$(MAKE) "ONE_TIMED_TARGET=ut" time_one
	@cat $(TIMELOG)
	@rm $(TIMELOG)
endif

clean_timed_target:
ifeq ($(TIMED_TARGET),)
	@echo "Error: You have to pass TIMED_TARGET to $@"
	false
else
	@echo "Build time:" > $(TIMELOG)
	$(MAKE) "ONE_TIMED_TARGET=clean" time_one
	$(MAKE) "ONE_TIMED_TARGET=$(TIMED_TARGET)" time_one
	@cat $(TIMELOG)
	@rm $(TIMELOG)
endif

# --------------------------------------------------------------------------------

build: arb
	$(MAKE) binlink preplib

all:
	@echo "Build time" > $(TIMELOG)
	@echo "$(SEP) $(MAKE) build"
	@$(TIMECMD) $(MAKE) build
	@echo $(SEP)
	@echo "made 'all' with success."
	@echo "to start arb enter 'arb'"
ifeq ($(UNIT_TESTS),1)
	@echo $(MAKE) run_tests
	@$(TIMECMD) $(MAKE) run_tests
endif
	@echo "$(SEP) $(MAKE) build [done]"
	@$(MAKE) save_test_no_error >/dev/null # just show hints
	@cat $(TIMELOG)
	@rm $(TIMELOG)


