
SUBS = \
		PROBE_GROUP/PROBE_GROUP.dummy \
		PROBE_GROUP_DESIGN/PROBE_GROUP_DESIGN.dummy \
		WORKER/WORKER.dummy \
		TREE_MERGE/TREE_MERGE.dummy \

DEST = bin

BINARIES = \
		$(DEST)/arb_probe_group \
		$(DEST)/arb_probe_group_design \
		$(DEST)/arb_probe_server_worker \
		$(DEST)/pgd_tree_merge \

LIBS = -lARBDB

CLIENT_SERVER_LIBS = \
		$(ARBHOME)/SERVERCNTRL/SERVERCNTRL.a \
		$(ARBHOME)/PROBE_COM/client.a \

ARCHS_PROBE_GROUP         = PROBE_GROUP/PROBE_GROUP.a
ARCHS_PROBE_GROUP_DESIGN  = PROBE_GROUP_DESIGN/PROBE_GROUP_DESIGN.a
ARCHS_PROBE_SERVER_WORKER = WORKER/WORKER.a
ARCHS_PGD_TREE_MERGE      = TREE_MERGE/TREE_MERGE.a

# --------------------------------------------------------------------------------

ifeq ('$(MAIN)','PROBE_SERVER.a')
		TARGET=$(MAIN)
else
		TARGET=help_local
endif

main_target : $(TARGET)

help_local:
	@echo ""
	@echo "    target      description"
	@echo "   ----------------------------------------------------------------------"
	@echo ""
	@echo "    binaries    builds needed binaries"
	@echo ""

binaries:
	(cd $(ARBHOME); make dball pt libs ps)

$(MAIN): $(SUBS) $(BINARIES)

$(BINARIES): makedestdir $(SUBS)

makedestdir:
		mkdir -p $(DEST)

$(DEST)/arb_probe_group:
	@$(ARBHOME)/SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_PROBE_GROUP) $(LIBS) $(SYSLIBS) $(CLIENT_SERVER_LIBS) || ( \
		echo Link $@ ; \
		echo $(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PROBE_GROUP) $(LIBS) $(SYSLIBS) $(CLIENT_SERVER_LIBS) ; \
		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PROBE_GROUP) $(LIBS) $(SYSLIBS) $(CLIENT_SERVER_LIBS) ; \
		)
#		@echo Link $@
#		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PROBE_GROUP) $(LIBS) $(SYSLIBS) $(CLIENT_SERVER_LIBS)

$(DEST)/arb_probe_group_design:
	@$(ARBHOME)/SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_PROBE_GROUP_DESIGN) $(LIBS) $(SYSLIBS) $(CLIENT_SERVER_LIBS) || ( \
		echo Link $@ ; \
		echo $(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PROBE_GROUP_DESIGN) $(LIBS) $(SYSLIBS) $(CLIENT_SERVER_LIBS) ; \
		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PROBE_GROUP_DESIGN) $(LIBS) $(SYSLIBS) $(CLIENT_SERVER_LIBS) ; \
		)
#		@echo Link $@
#		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PROBE_GROUP_DESIGN) $(LIBS) $(SYSLIBS) $(CLIENT_SERVER_LIBS)

$(DEST)/arb_probe_server_worker:
	@$(ARBHOME)/SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_PROBE_SERVER_WORKER) $(LIBS) $(SYSLIBS) || ( \
		echo Link $@ ; \
		echo $(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PROBE_SERVER_WORKER) $(LIBS) $(SYSLIBS) ; \
		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PROBE_SERVER_WORKER) $(LIBS) $(SYSLIBS) ; \
		)
#		@echo Link $@
#		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PROBE_SERVER_WORKER) $(LIBS) $(SYSLIBS)

$(DEST)/pgd_tree_merge:
	@$(ARBHOME)/SOURCE_TOOLS/binuptodate.pl $@ $(ARCHS_PGD_TREE_MERGE) $(LIBS) $(SYSLIBS) || ( \
		echo Link $@ ; \
		echo $(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PGD_TREE_MERGE) $(LIBS) $(SYSLIBS) ; \
		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PGD_TREE_MERGE) $(LIBS) $(SYSLIBS) ; \
		)
#		@echo Link $@
#		$(CPP) $(lflags) -o $@ $(LIBPATH) $(ARCHS_PGD_TREE_MERGE) $(LIBS) $(SYSLIBS)

depends: $(SUBS:.dummy=.depend)
clean: $(SUBS:.dummy=.clean)

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
	@((($(MAKE) -C $(@D) -r \
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
		"MAIN = $(@F:.dummy=.a)" \
		>$(@D).log 2>&1 && cat $(@D).log) || (cat $(@D).log; false)) && (rm $(@D).log))

%.clean:
	@echo "$(ARBHOME)/PROBE_SERVER/$(@D)/Makefile:1: here"
	@$(MAKE) -C $(@D) \
		"OPENGL=$(OPENGL)" \
		clean

# the end of the above command avoids that the output of parallel make calls gets mixed up 

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl
