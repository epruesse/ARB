
ARCHS_ALL = \
	CLUSTAL/CLUSTAL.dummy  \
	SUPPORT/SUPPORT.dummy \
	MOLPHY/MOLPHY.dummy \
	AxML/AxML.dummy \

# --------------------------------------------------------------------------------
# All packages using xview go here:

ARCHS_XVIEW = \
	HGL_SRC/HGL_SRC.dummy \
	LOOPTOOL/LOOPTOOL.dummy \
	CORE/CORE.dummy \

# --------------------------------------------------------------------------------
# If a package is available as debian package it should go here

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

SUB_CPP=$(CPP:-W -Wall=-w)
SUB_ACC=$(ACC:-W -Wall=-w)

# --------------------------------------------------------------------------------

$(MAIN): $(ARCHS)

depend: $(ARCHS:.dummy=.depend)

%.depend:
	@$(MAKE) -C $(@D) -r \
		"LD_LIBRARY_PATH  = ${LD_LIBRARY_PATH}" \
		"MAKEDEPENDINC = $(MAKEDEPENDINC)" \
		"MAKEDEPEND=$(MAKEDEPEND)" \
		depend;
	cat $(@D)/Makefile \
		| sed	-e "s/\/[^ 	]*\/DUMMYINC\/[^ 	]*\.h//g" \
			-e "s/\/usr\/[^ 	]*\.h//g" \
			-e "s&$(ARBHOME)&\\\$$(ARBHOME)&g" \
			-e '/^[A-Za-z][A-Za-z0-9_]*\.o:[ ]*$$/d' \
		>$(@D)/Makefile.2
	@mv $(@D)/Makefile.2 $(@D)/Makefile

%.dummy:
	@$(MAKE) -C $(@D) -r \
		"MAKE = $(MAKE)" \
		"ARBHOME = $(ARBHOME)" "cflags = $(cflags) -D_ARB_$(@D:/=)" "lflags = $(lflags)" \
		"CPPINCLUDES = $(CPPINCLUDES)" "AINCLUDES = $(AINCLUDES)" \
		"CPP = $(SUB_CPP)" "ACC = $(SUB_ACC)" \
		"CCLIB = $(CCLIB)" "CCPLIB = $(CCPLIB)" \
		"AR = $(AR)" "ARLIB = $(ARLIB)" \
		"LIBPATH = $(LIBPATH)" "SYSLIBS = $(SYSLIBS)" \
		"SHARED_LIB_SUFFIX = $(SHARED_LIB_SUFFIX)" \
		"XHOME = $(XHOME)" \
		"LD_LIBRARY_PATH  = $(LD_LIBRARY_PATH)" \
		"ARB  = yes" \
		"MAIN = $(@F:.dummy=.a)"
