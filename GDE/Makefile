
ifndef OPENWINHOME
ifndef DEBIAN
ARCHS = CLUSTAL/CLUSTAL.dummy  \
	CLUSTALW/CLUSTALW.dummy  \
	PHYLIP/PHYLIP.dummy \
	SUPPORT/SUPPORT.dummy \
	MOLPHY/MOLPHY.dummy \
	TREEPUZZLE/TREEPUZZLE.dummy \
	FASTDNAML/FASTDNAML.dummy
else
export OPENWINHOME = /usr/X11R6
ARCHS = CLUSTAL/CLUSTAL.dummy  \
	SUPPORT/SUPPORT.dummy \
	HGL_SRC/HGL_SRC.dummy \
	MOLPHY/MOLPHY.dummy \
	LOOPTOOL/LOOPTOOL.dummy \
	CORE/CORE.dummy
## Debian has the following own packages:
#	CLUSTALW/CLUSTALW.dummy  \
#	PHYLIP/PHYLIP.dummy \
#	FASTDNAML/FASTDNAML.dummy \
#	TREEPUZZLE/TREEPUZZLE.dummy \
#
endif
else
ARCHS = CLUSTAL/CLUSTAL.dummy  \
	CLUSTALW/CLUSTALW.dummy  \
	PHYLIP/PHYLIP.dummy \
	SUPPORT/SUPPORT.dummy \
	HGL_SRC/HGL_SRC.dummy \
	FASTDNAML/FASTDNAML.dummy \
	MOLPHY/MOLPHY.dummy \
	LOOPTOOL/LOOPTOOL.dummy \
	TREEPUZZLE/TREEPUZZLE.dummy \
	CORE/CORE.dummy
endif

#ZUKER/ZUKER.dummy

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

#			-e "s&/trance1/ARB/source/ARB/&\\\$$(ARBHOME)/&g" \

%.dummy:
	@$(MAKE) -C $(@D) -r \
		"MAKE = $(MAKE)" \
		"ARBHOME = $(ARBHOME)" "cflags = $(cflags) -D_ARB_$(@D:/=)" "lflags = $(lflags)" \
		"CPPINCLUDES = $(CPPINCLUDES)" "AINCLUDES = $(AINCLUDES)" \
		"CPP = $(CPP)" "ACC = $(ACC)" \
		"CCLIB = $(CCLIB)" "CCPLIB = $(CCPLIB)" \
		"AR = $(AR)" "ARLIB = $(ARLIB)" \
		"LIBPATH = $(LIBPATH)" "SYSLIBS = $(SYSLIBS)" \
		"SHARED_LIB_SUFFIX = $(SHARED_LIB_SUFFIX)" \
		"XHOME = $(XHOME)" \
		"LD_LIBRARY_PATH  = $(LD_LIBRARY_PATH)" \
		"MAIN = $(@F:.dummy=.a)"
