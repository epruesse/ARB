
SUBS = \
		PROBE_GROUP/PROBE_GROUP.dummy \
		PROBE_GROUP_DESIGN/PROBE_GROUP_DESIGN.dummy \

DEST = bin

BINARIES = \
		$(DEST)/arb_probe_group \
		$(DEST)/arb_probe_group_design \

LIBS = -lARBDB \
		$(ARBHOME)/SERVERCNTRL/SERVERCNTRL.a \
		$(ARBHOME)/PROBE_COM/client.a \


ARCHS_PROBE_GROUP        = PROBE_GROUP/PROBE_GROUP.a
ARCHS_PROBE_GROUP_DESIGN = PROBE_GROUP_DESIGN/PROBE_GROUP_DESIGN.a

# --------------------------------------------------------------------------------

$(MAIN): $(SUBS) $(BINARIES)

$(BINARIES): makedestdir $(SUBS)

makedestdir:
		mkdir -p $(DEST)

$(DEST)/arb_probe_group:
		@echo Link $@
		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PROBE_GROUP) $(LIBS)

$(DEST)/arb_probe_group_design:
		@echo Link $@
		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PROBE_GROUP_DESIGN) $(LIBS)

depends: $(SUBS:.dummy=.depend)

%.depend:
	@cp -p $(@D)/Makefile $(@D)/Makefile.old # save old Makefile
	@$(MAKE) -C $(@D) -r \
		"LD_LIBRARY_PATH  = ${LD_LIBRARY_PATH}" \
		"MAKEDEPENDFLAGS = $(MAKEDEPENDFLAGS)" \
		"MAKEDEPEND=$(MAKEDEPEND)" \
		"ARBHOME=$(ARBHOME)" \
		depends;
	@grep "^# DO NOT DELETE" $(@D)/Makefile >/dev/null # check whether sub Makefile has dependencies
	@cat $(@D)/Makefile | sed \
			-e "s&$(ARBHOME)&\\\$$(ARBHOME)&g" \
			-e "s/[ /t]\.\// /g" \
		>$(@D)/Makefile.2
	@mv $(@D)/Makefile.old $(@D)/Makefile # restore old Makefile
	@$(ARBHOME)/SOURCE_TOOLS/mv_if_diff $(@D)/Makefile.2 $(@D)/Makefile # update Makefile if changed

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
		"ARB  = yes" \
		"MAIN = $(@F:.dummy=.a)"

# DO NOT DELETE
