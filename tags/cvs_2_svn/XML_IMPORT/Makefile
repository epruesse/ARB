
ARCHS = \
	IMP_TREE/IMP_TREE.dummy \

# --------------------------------------------------------------------------------
SAX_INCLUDES        = -I. -I${XERCESCROOT}/include 
SAX_LIB_SEARCH_PATH = -L${XERCESCROOT}/lib
SAX_LIBRARY_NAME    = -lxerces-c

SUB_CFLAGS=$(cflags) $(SAX_INCLUDES) -DXML_USE_PTHREADS
SUB_LFLAGS=$(lflags) $(SAX_LIB_SEARCH_PATH) $(SAX_LIBRARY_NAME)

# currently this is not compiled (ask yadhu why?)

kaputt:
	@echo "lflags no longer supported. directly change linker commands above"
	false

# --------------------------------------------------------------------------------


$(MAIN): $(ARCHS) kaputt

depends: $(ARCHS:.dummy=.depend)

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
	@$(MAKE) -C $(@D) -r \
		"ACC = $(ACC)" \
		"AINCLUDES = $(AINCLUDES)" \
		"ARB  = yes" \
		"ARBHOME = $(ARBHOME)" \
		"CCPLIB = $(CCPLIB)" \
		"CPP = $(CPP)" \
		"CPPINCLUDES = $(CPPINCLUDES)" \
		"LD_LIBRARY_PATH  = $(LD_LIBRARY_PATH)" \
		"LIBPATH = $(LIBPATH)" \
		"LINK_EXECUTABLE = $(LINK_EXECUTABLE)" \
		"LINK_STATIC_LIB = $(LINK_STATIC_LIB)" \
		"MAIN = $(@F:.dummy=.a)" \
		"MAKE = $(MAKE)" \
		"SHARED_LIB_SUFFIX = $(SHARED_LIB_SUFFIX)" \
		"SYSLIBS = $(SYSLIBS)" \
		"XHOME = $(XHOME)" \
		"cflags = $(SUB_CFLAGS) -DIN_ARB_$(@D:/=)" \

#		"lflags = $(SUB_LFLAGS)" 


# DO NOT DELETE
