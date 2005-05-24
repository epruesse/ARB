
ARCHS_ALL = \
	HELIX/HELIX.dummy  \
	AW_HELIX/AW_HELIX.dummy  \
	AW_NAME/AW_NAME.dummy  \
	DB_SCANNER/DB_SCANNER.dummy  \

# --------------------------------------------------------------------------------
# If a package is available as debian package it should go here

ARCHS_NON_DEBIAN = \

# --------------------------------------------------------------------------------

ifdef DEBIAN
ARCHS = $(ARCHS_ALL) 
else
ARCHS = $(ARCHS_ALL) $(ARCHS_NON_DEBIAN)
endif

# --------------------------------------------------------------------------------
# warnings in this subtree?

RAISE_WARNINGS=1

ifeq ($(RAISE_WARNINGS),1)
SUB_CPP=$(CPP)
SUB_ACC=$(ACC)
else
SUB_CPP=$(CPP:-W -Wall=-w)
SUB_ACC=$(ACC:-W -Wall=-w)
endif

# --------------------------------------------------------------------------------

$(MAIN): $(ARCHS)

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
		"MAKE = $(MAKE)" \
		"ARBHOME = $(ARBHOME)" "cflags = $(cflags) -D_ARB_$(@D:/=)" "lflags = $(lflags)" \
		"CPPINCLUDES = $(CPPINCLUDES)" "AINCLUDES = $(AINCLUDES)" \
		"CPP = $(SUB_CPP)" "ACC = $(SUB_ACC)" \
		"CCLIB = $(CCLIB)" "CCPLIB = $(CCPLIB)" \
		"AR = $(AR)" "ARLIB = $(ARLIB)" \
		"LIBPATH = $(LIBPATH)" "SYSLIBS = $(SYSLIBS)" \
		"SHARED_LIB_SUFFIX = $(SHARED_LIB_SUFFIX)" \
		"XHOME = $(XHOME)" "XLIBS = $(XLIBS)" \
		"LD_LIBRARY_PATH  = $(LD_LIBRARY_PATH)" \
		"ARB  = yes" \
		"MAIN = $(@F:.dummy=.a)"

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl
