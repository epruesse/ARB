
ifndef OPENWINHOME
ARCHS = CLUSTAL/CLUSTAL.dummy  \
	CLUSTALW/CLUSTALW.dummy  \
	PHYLIP/PHYLIP.dummy \
	SUPPORT/SUPPORT.dummy \
	MOLPHY/MOLPHY.dummy \
	TREEPUZZLE/TREEPUZZLE.dummy \
	FASTDNAML/FASTDNAML.dummy 
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
		"MAKEDEPENDINC = $(MAKEDEPENDINC)" \
		"MAKEDEPEND=$(MAKEDEPEND)" depend;
	@cat $(@D)/Makefile \
		| sed	-e "s/\/[^ 	]*\/DUMMYINC\/[^ 	]*\.h//g" \
			-e "s/\/usr\/include\/[^ 	]*\.h//g" \
			-e "s/\/[^ 	]*\/INCLUDE/\\\$$(ARBHOME)\/INCLUDE/g" \
		>$(@D)/Makefile.2
	@mv $(@D)/Makefile.2 $(@D)/Makefile

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
