.SUFFIXES: .o .cxx .depend

CPP_OBJECTS =	PT_new_design.o PT_family.o PT_prefixtree.o PT_main.o PT_io.o \
		PT_etc.o PT_secundaer.o PT_buildtree.o PT_match.o PT_findEx.o
C_OBJECTS = # BI_helix.o
OBJECTS = $(CPP_OBJECTS) $(C_OBJECTS)

$(MAIN): $(OBJECTS) 
	$(AR) $(MAIN) $(OBJECTS)

.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES)  -I./../probe_bank/COM

.cxx.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES) -I./../probe_bank/COM

proto:
	../MAKEBIN/aisc_mkpt -E $(OBJECTS:.o=.cxx) probe_tree.hxx >pt_prototypes.h.tmp
	../SOURCE_TOOLS/mv_if_diff pt_prototypes.h.tmp pt_prototypes.h

clean:
	rm -f $(OBJECTS) *.a

DEPENDS = $(OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

PT_buildtree.o: probe.h
PT_buildtree.o: probe_tree.hxx
PT_buildtree.o: pt_prototypes.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h

PT_etc.o: probe.h
PT_etc.o: pt_prototypes.h
PT_etc.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_etc.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_etc.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_etc.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_etc.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_etc.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_etc.o: $(ARBHOME)/INCLUDE/struct_man.h

PT_family.o: probe.h
PT_family.o: probe_tree.hxx
PT_family.o: pt_prototypes.h
PT_family.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_family.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_family.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_family.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_family.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_family.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_family.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_family.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_family.o: $(ARBHOME)/INCLUDE/struct_man.h

PT_findEx.o: probe.h
PT_findEx.o: probe_tree.hxx
PT_findEx.o: pt_prototypes.h
PT_findEx.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_findEx.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_findEx.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_findEx.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_findEx.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_findEx.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h

PT_io.o: probe.h
PT_io.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_io.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_io.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_io.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_io.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_io.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
PT_io.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_io.o: $(ARBHOME)/INCLUDE/PT_server.h

PT_main.o: probe.h
PT_main.o: pt_prototypes.h
PT_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_main.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
PT_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_main.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_main.o: $(ARBHOME)/INCLUDE/client.h
PT_main.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_main.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_main.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_main.o: $(ARBHOME)/INCLUDE/server.h
PT_main.o: $(ARBHOME)/INCLUDE/servercntrl.h
PT_main.o: $(ARBHOME)/INCLUDE/struct_man.h

PT_match.o: probe.h
PT_match.o: probe_tree.hxx
PT_match.o: pt_prototypes.h
PT_match.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_match.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_match.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_match.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_match.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_match.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_match.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_match.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_match.o: $(ARBHOME)/INCLUDE/struct_man.h

PT_new_design.o: probe.h
PT_new_design.o: probe_tree.hxx
PT_new_design.o: pt_prototypes.h
PT_new_design.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_new_design.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_new_design.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_new_design.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_new_design.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_new_design.o: $(ARBHOME)/INCLUDE/struct_man.h

PT_prefixtree.o: probe.h
PT_prefixtree.o: probe_tree.hxx
PT_prefixtree.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/PT_server.h

PT_secundaer.o: probe.h
PT_secundaer.o: pt_prototypes.h
PT_secundaer.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_secundaer.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_secundaer.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_secundaer.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_secundaer.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_secundaer.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
