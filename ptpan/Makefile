.SUFFIXES: .o .cxx .depend
OBJECTS = PT_new_design.o PT_main.o PT_io.o \
	PT_etc.o PT_family.o\
	PT_lowlevel.o PT_huffman.o PT_treepack.o \
	PT_buildtree.o PT_match.o PT_findEx.o \
	PT_cachehandler.o PT_hashing.o \
	dlist.o
AOBJECTS = #BI_helix.o

$(MAIN): $(OBJECTS) $(AOBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS) $(AOBJECTS)
.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES)  -I./../probe_bank/COM
.cxx.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES) -I./../probe_bank/COM
proto:
	rm -f pt_prototypes.h
	../MAKEBIN/aisc_mkpt -E $(OBJECTS:.o=.cxx) >pt_prototypes.h

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

dlist.o: dlist.h
dlist.o: types.h

PT_buildtree.o: dlist.h
PT_buildtree.o: globalprefs.h
PT_buildtree.o: hooks.h
PT_buildtree.o: probe.h
PT_buildtree.o: pt_manualprotos.h
PT_buildtree.o: pt_prototypes.h
PT_buildtree.o: types.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/attributes.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h

PT_cachehandler.o: dlist.h
PT_cachehandler.o: globalprefs.h
PT_cachehandler.o: hooks.h
PT_cachehandler.o: probe.h
PT_cachehandler.o: pt_manualprotos.h
PT_cachehandler.o: pt_prototypes.h
PT_cachehandler.o: types.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/attributes.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h

PT_etc.o: dlist.h
PT_etc.o: globalprefs.h
PT_etc.o: hooks.h
PT_etc.o: probe.h
PT_etc.o: pt_manualprotos.h
PT_etc.o: pt_prototypes.h
PT_etc.o: types.h
PT_etc.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_etc.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_etc.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_etc.o: $(ARBHOME)/INCLUDE/attributes.h
PT_etc.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_etc.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_etc.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_etc.o: $(ARBHOME)/INCLUDE/struct_man.h

PT_family.o: dlist.h
PT_family.o: globalprefs.h
PT_family.o: hooks.h
PT_family.o: probe.h
PT_family.o: pt_manualprotos.h
PT_family.o: pt_prototypes.h
PT_family.o: types.h
PT_family.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_family.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_family.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_family.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_family.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_family.o: $(ARBHOME)/INCLUDE/attributes.h
PT_family.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_family.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_family.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_family.o: $(ARBHOME)/INCLUDE/struct_man.h

PT_findEx.o: dlist.h
PT_findEx.o: globalprefs.h
PT_findEx.o: hooks.h
PT_findEx.o: probe.h
PT_findEx.o: pt_manualprotos.h
PT_findEx.o: pt_prototypes.h
PT_findEx.o: types.h
PT_findEx.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_findEx.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_findEx.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_findEx.o: $(ARBHOME)/INCLUDE/attributes.h
PT_findEx.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_findEx.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_findEx.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h

PT_hashing.o: dlist.h
PT_hashing.o: globalprefs.h
PT_hashing.o: hooks.h
PT_hashing.o: probe.h
PT_hashing.o: pt_manualprotos.h
PT_hashing.o: pt_prototypes.h
PT_hashing.o: types.h
PT_hashing.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_hashing.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_hashing.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_hashing.o: $(ARBHOME)/INCLUDE/attributes.h
PT_hashing.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_hashing.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_hashing.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h

PT_huffman.o: dlist.h
PT_huffman.o: globalprefs.h
PT_huffman.o: hooks.h
PT_huffman.o: probe.h
PT_huffman.o: pt_manualprotos.h
PT_huffman.o: pt_prototypes.h
PT_huffman.o: types.h
PT_huffman.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_huffman.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_huffman.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_huffman.o: $(ARBHOME)/INCLUDE/attributes.h
PT_huffman.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_huffman.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_huffman.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h

PT_io.o: dlist.h
PT_io.o: globalprefs.h
PT_io.o: hooks.h
PT_io.o: probe.h
PT_io.o: pt_manualprotos.h
PT_io.o: pt_prototypes.h
PT_io.o: types.h
PT_io.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_io.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_io.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_io.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_io.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_io.o: $(ARBHOME)/INCLUDE/attributes.h
PT_io.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
PT_io.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_io.o: $(ARBHOME)/INCLUDE/PT_server.h

PT_lowlevel.o: dlist.h
PT_lowlevel.o: globalprefs.h
PT_lowlevel.o: hooks.h
PT_lowlevel.o: probe.h
PT_lowlevel.o: pt_manualprotos.h
PT_lowlevel.o: pt_prototypes.h
PT_lowlevel.o: types.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/attributes.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h

PT_main.o: dlist.h
PT_main.o: globalprefs.h
PT_main.o: hooks.h
PT_main.o: probe.h
PT_main.o: pt_manualprotos.h
PT_main.o: pt_prototypes.h
PT_main.o: types.h
PT_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_main.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
PT_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_main.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_main.o: $(ARBHOME)/INCLUDE/attributes.h
PT_main.o: $(ARBHOME)/INCLUDE/client.h
PT_main.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_main.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_main.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_main.o: $(ARBHOME)/INCLUDE/server.h
PT_main.o: $(ARBHOME)/INCLUDE/servercntrl.h
PT_main.o: $(ARBHOME)/INCLUDE/struct_man.h

PT_match.o: dlist.h
PT_match.o: globalprefs.h
PT_match.o: hooks.h
PT_match.o: probe.h
PT_match.o: pt_manualprotos.h
PT_match.o: pt_prototypes.h
PT_match.o: types.h
PT_match.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_match.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_match.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_match.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_match.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_match.o: $(ARBHOME)/INCLUDE/attributes.h
PT_match.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_match.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_match.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_match.o: $(ARBHOME)/INCLUDE/struct_man.h

PT_new_design.o: dlist.h
PT_new_design.o: globalprefs.h
PT_new_design.o: hooks.h
PT_new_design.o: probe.h
PT_new_design.o: pt_manualprotos.h
PT_new_design.o: pt_prototypes.h
PT_new_design.o: types.h
PT_new_design.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_new_design.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_new_design.o: $(ARBHOME)/INCLUDE/attributes.h
PT_new_design.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_new_design.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_new_design.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_new_design.o: $(ARBHOME)/INCLUDE/struct_man.h

PT_treepack.o: dlist.h
PT_treepack.o: globalprefs.h
PT_treepack.o: hooks.h
PT_treepack.o: probe.h
PT_treepack.o: pt_manualprotos.h
PT_treepack.o: pt_prototypes.h
PT_treepack.o: types.h
PT_treepack.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_treepack.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_treepack.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_treepack.o: $(ARBHOME)/INCLUDE/attributes.h
PT_treepack.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_treepack.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_treepack.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
