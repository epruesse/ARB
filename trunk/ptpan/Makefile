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
depend: $(DEPENDS)
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

PT_new_design.o:  $(ARBHOME)/INCLUDE/PT_server.h
PT_new_design.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_new_design.o: $(ARBHOME)/INCLUDE/struct_man.h probe.h globalprefs.h
PT_new_design.o: types.h dlist.h hooks.h $(ARBHOME)/INCLUDE/PT_com.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_new_design.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_new_design.o: $(ARBHOME)/INCLUDE/ad_t_prot.h pt_prototypes.h
PT_main.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_main.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h probe.h
PT_main.o: globalprefs.h types.h dlist.h hooks.h
PT_main.o: $(ARBHOME)/INCLUDE/PT_com.h $(ARBHOME)/INCLUDE/arbdb.h
PT_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_main.o: $(ARBHOME)/INCLUDE/ad_prot.h pt_prototypes.h
PT_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_main.o: $(ARBHOME)/INCLUDE/servercntrl.h
PT_main.o: $(ARBHOME)/INCLUDE/server.h
PT_main.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
PT_main.o: $(ARBHOME)/INCLUDE/client.h
PT_main.o: $(ARBHOME)/INCLUDE/struct_man.h
PT_io.o:  $(ARBHOME)/INCLUDE/PT_server.h probe.h
PT_io.o: globalprefs.h types.h dlist.h hooks.h
PT_io.o: $(ARBHOME)/INCLUDE/PT_com.h $(ARBHOME)/INCLUDE/arbdb.h
PT_io.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_io.o: $(ARBHOME)/INCLUDE/ad_prot.h pt_prototypes.h
PT_io.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_io.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_io.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
PT_etc.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_etc.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h probe.h
PT_etc.o: globalprefs.h types.h dlist.h hooks.h
PT_etc.o: $(ARBHOME)/INCLUDE/PT_com.h $(ARBHOME)/INCLUDE/arbdb.h
PT_etc.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_etc.o: $(ARBHOME)/INCLUDE/ad_prot.h pt_prototypes.h
PT_etc.o: $(ARBHOME)/INCLUDE/struct_man.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h probe.h
PT_lowlevel.o: globalprefs.h types.h dlist.h hooks.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/ad_prot.h 
PT_lowlevel.o: pt_prototypes.h
PT_huffman.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_huffman.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h probe.h
PT_huffman.o: globalprefs.h types.h dlist.h hooks.h
PT_huffman.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_huffman.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_huffman.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_huffman.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_huffman.o: pt_prototypes.h
PT_treepack.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_treepack.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h probe.h
PT_treepack.o: globalprefs.h types.h dlist.h hooks.h
PT_treepack.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_treepack.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_treepack.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_treepack.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_treepack.o: pt_prototypes.h 
PT_buildtree.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h probe.h
PT_buildtree.o: globalprefs.h types.h dlist.h hooks.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/ad_prot.h 
PT_buildtree.o: pt_prototypes.h
PT_match.o:  $(ARBHOME)/INCLUDE/PT_server.h
PT_match.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_match.o: $(ARBHOME)/INCLUDE/struct_man.h probe.h globalprefs.h
PT_match.o: types.h dlist.h hooks.h $(ARBHOME)/INCLUDE/PT_com.h
PT_match.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_match.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_match.o: $(ARBHOME)/INCLUDE/ad_prot.h pt_prototypes.h
PT_match.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_match.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_findEx.o:  $(ARBHOME)/INCLUDE/PT_server.h
PT_findEx.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h probe.h
PT_findEx.o: globalprefs.h types.h dlist.h hooks.h
PT_findEx.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_findEx.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_findEx.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_findEx.o: $(ARBHOME)/INCLUDE/ad_prot.h 
PT_findEx.o: pt_prototypes.h
PT_cachehandler.o:  $(ARBHOME)/INCLUDE/PT_server.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h probe.h
PT_cachehandler.o: globalprefs.h types.h dlist.h hooks.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/ad_prot.h pt_prototypes.h
dlist.o:   dlist.h types.h
hooks.o: hooks.h dlist.h types.h
