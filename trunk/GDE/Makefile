
ARCHS_ALL = \
	CLUSTAL/CLUSTAL.dummy  \
	SUPPORT/SUPPORT.dummy \
	MOLPHY/MOLPHY.dummy \
	AxML/AxML.dummy \
	RAxML/RAxML.dummy \
	PHYML/PHYML.dummy \

# --------------------------------------------------------------------------------
# All packages using xview go here:

ARCHS_XVIEW = \
	CORE/CORE.dummy \

# deprecated:
#	HGL_SRC/HGL_SRC.dummy
#	LOOPTOOL/LOOPTOOL.dummy

# --------------------------------------------------------------------------------
# If a package is available as debian package it should go here
# 
# Note: I'm not sure whether this is a good idea. Due to limited manpower it often takes
# longer until ARB works with newer (incompatible) versions of these tools. --ralf

ARCHS_NON_DEBIAN = \
	CLUSTALW/CLUSTALW.dummy  \
	PHYLIP/PHYLIP.dummy \
	TREEPUZZLE/TREEPUZZLE.dummy \
	FASTDNAML/FASTDNAML.dummy \

# --------------------------------------------------------------------------------

ifdef OPENWINHOME
ARCHS = $(ARCHS_ALL) $(ARCHS_NON_DEBIAN) $(ARCHS_XVIEW)
else
ifdef DEBIAN
ARCHS = $(ARCHS_ALL) $(ARCHS_XVIEW)
else
ARCHS = $(ARCHS_ALL) $(ARCHS_NON_DEBIAN)
endif
endif

# --------------------------------------------------------------------------------
# no warnings in this subtree

RAISE_WARNINGS=0

ifeq ('$(RAISE_WARNINGS)','0')
SUB_CPP=$(subst -W -Wall,-w,$(CPP))
SUB_ACC=$(subst -W -Wall,-w,$(ACC))
else
SUB_CPP=$(CPP)
SUB_ACC=$(ACC)
endif

# --------------------------------------------------------------------------------

$(MAIN):
ifeq ('$(RAISE_WARNINGS)','0')
	@echo ----------------- non-ARB code: warnings disabled
endif
	$(MAKE) $(ARCHS)
ifeq ('$(RAISE_WARNINGS)','0')
	@echo ----------------- warnings enabled again
endif

proto:
	$(MAKE) -C CORE proto

depends: $(ARCHS:.dummy=.depend)

clean: $(ARCHS:.dummy=.clean)

%.depend:
	@cp -p $(@D)/Makefile $(@D)/Makefile.old # save old Makefile
	@$(MAKE) -C $(@D) -r \
		"LD_LIBRARY_PATH  = ${LD_LIBRARY_PATH}" \
		"MAKEDEPENDFLAGS = $(MAKEDEPENDFLAGS)" \
		"MAKEDEPEND=$(MAKEDEPEND)" \
		"ARBHOME=$(ARBHOME)" \
		depends;
	@grep "^# DO NOT DELETE" $(@D)/Makefile >/dev/null # check whether sub Makefile has dependencies
	@cat $(@D)/Makefile \
		| ../SOURCE_TOOLS/fix_depends.pl \
		>$(@D)/Makefile.2
	@mv $(@D)/Makefile.old $(@D)/Makefile # restore old Makefile
	@$(ARBHOME)/SOURCE_TOOLS/mv_if_diff $(@D)/Makefile.2 $(@D)/Makefile # update Makefile if changed

%.dummy:
	@(( \
	    $(MAKE) -C $(@D) -r \
		"ACC = $(SUB_ACC)" \
		"AINCLUDES = $(AINCLUDES)" \
		"AR = $(AR)" \
		"ARB  = yes" \
		"ARBHOME = $(ARBHOME)" \
		"ARLIB = $(ARLIB)" \
		"CCLIB = $(CCLIB)" \
		"CCPLIB = $(CCPLIB)" \
		"CPP = $(SUB_CPP)" \
		"CPPINCLUDES = $(CPPINCLUDES)" \
		"LD_LIBRARY_PATH  = $(LD_LIBRARY_PATH)" \
		"LIBPATH = $(LIBPATH)" \
		"MAIN = $(@F:.dummy=.a)" \
		"MAKE = $(MAKE)" \
		"SHARED_LIB_SUFFIX = $(SHARED_LIB_SUFFIX)" \
		"SYSLIBS = $(SYSLIBS)" \
		"XHOME = $(XHOME)" \
		"XLIBS = $(XLIBS)" \
		"cflags = $(cflags) -DIN_ARB_$(@D:/=)" \
		"lflags = $(lflags)" \
		>$(@D).log 2>&1 && (cat $(@D).log;rm $(@D).log)) || (cat $(@D).log;rm $(@D).log;false))

%.clean:
	@$(MAKE) -C $(@D) \
		"ARB  = yes" \
		"OPENGL=$(OPENGL)" \
		clean

# the end of the above command avoids that the output of parallel make calls gets mixed up 

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl
