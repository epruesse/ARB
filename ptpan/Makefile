.SUFFIXES: .o .cxx .depend
OBJECTS = PTP_new_design.o PTP_main.o PTP_io.o \
	PTP_etc.o PTP_family.o\
	PT_lowlevel.o PT_huffman.o PT_treepack.o \
	PTP_buildtree.o PTP_match.o PTP_findEx.o \
	PT_cachehandler.o PT_hashing.o \
	dlist.o
AOBJECTS = #BI_helix.o

# --------------------------------------------------------------------------------
# no warnings for ptpan (code is rotten in public repository)

RAISE_WARNINGS=0

ifeq ('$(RAISE_WARNINGS)','0')
pan_cflags:=$(subst -W -Wall,-w,$(cflags))
else
pan_cflags:=$(cflags)
endif

# --------------------------------------------------------------------------------

$(MAIN): $(OBJECTS) $(AOBJECTS) Makefile
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS) $(AOBJECTS)
ifeq ('$(RAISE_WARNINGS)','0')
	@echo ----------------- warnings enabled again
endif

$(OBJECTS): Makefile

.c.o:
	$(ACC) $(pan_cflags) -c $< $(AINCLUDES)  -I./../probe_bank/COM $(POST_COMPILE)
.cxx.o:
	$(CPP) $(pan_cflags) -c $<  $(CPPINCLUDES) -I./../probe_bank/COM $(POST_COMPILE)
proto:
	../MAKEBIN/aisc_mkpt -E -z -A -w pt_prototypes.h $(OBJECTS:.o=.cxx) >pt_prototypes.h.tmp
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
	-@rm Makefile.bak # get rid of bak (generated for some dubious reason only in this directory)
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

dlist.o: dlist.h
dlist.o: types.h

PT_cachehandler.o: dlist.h
PT_cachehandler.o: globalprefs.h
PT_cachehandler.o: hooks.h
PT_cachehandler.o: pt_prototypes.h
PT_cachehandler.o: ptpan.h
PT_cachehandler.o: types.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/aisc_global.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/arb_core.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/arb_error.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/arb_msg.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/arb_string.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/arbtools.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/attributes.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/bytestring.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/dupstr.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/smartptr.h
PT_cachehandler.o: $(ARBHOME)/INCLUDE/test_global.h

PT_hashing.o: dlist.h
PT_hashing.o: globalprefs.h
PT_hashing.o: hooks.h
PT_hashing.o: pt_prototypes.h
PT_hashing.o: ptpan.h
PT_hashing.o: types.h
PT_hashing.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_hashing.o: $(ARBHOME)/INCLUDE/aisc_global.h
PT_hashing.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_hashing.o: $(ARBHOME)/INCLUDE/arb_core.h
PT_hashing.o: $(ARBHOME)/INCLUDE/arb_error.h
PT_hashing.o: $(ARBHOME)/INCLUDE/arb_msg.h
PT_hashing.o: $(ARBHOME)/INCLUDE/arb_string.h
PT_hashing.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_hashing.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PT_hashing.o: $(ARBHOME)/INCLUDE/arbtools.h
PT_hashing.o: $(ARBHOME)/INCLUDE/attributes.h
PT_hashing.o: $(ARBHOME)/INCLUDE/bytestring.h
PT_hashing.o: $(ARBHOME)/INCLUDE/dupstr.h
PT_hashing.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_hashing.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_hashing.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_hashing.o: $(ARBHOME)/INCLUDE/smartptr.h
PT_hashing.o: $(ARBHOME)/INCLUDE/test_global.h

PT_huffman.o: dlist.h
PT_huffman.o: globalprefs.h
PT_huffman.o: hooks.h
PT_huffman.o: pt_prototypes.h
PT_huffman.o: ptpan.h
PT_huffman.o: types.h
PT_huffman.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_huffman.o: $(ARBHOME)/INCLUDE/aisc_global.h
PT_huffman.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_huffman.o: $(ARBHOME)/INCLUDE/arb_core.h
PT_huffman.o: $(ARBHOME)/INCLUDE/arb_error.h
PT_huffman.o: $(ARBHOME)/INCLUDE/arb_msg.h
PT_huffman.o: $(ARBHOME)/INCLUDE/arb_string.h
PT_huffman.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_huffman.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PT_huffman.o: $(ARBHOME)/INCLUDE/arbtools.h
PT_huffman.o: $(ARBHOME)/INCLUDE/attributes.h
PT_huffman.o: $(ARBHOME)/INCLUDE/bytestring.h
PT_huffman.o: $(ARBHOME)/INCLUDE/dupstr.h
PT_huffman.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_huffman.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_huffman.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_huffman.o: $(ARBHOME)/INCLUDE/smartptr.h
PT_huffman.o: $(ARBHOME)/INCLUDE/test_global.h

PT_lowlevel.o: dlist.h
PT_lowlevel.o: globalprefs.h
PT_lowlevel.o: hooks.h
PT_lowlevel.o: pt_prototypes.h
PT_lowlevel.o: ptpan.h
PT_lowlevel.o: types.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/aisc_global.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/arb_core.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/arb_error.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/arb_msg.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/arb_string.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/arbtools.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/attributes.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/bytestring.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/dupstr.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/smartptr.h
PT_lowlevel.o: $(ARBHOME)/INCLUDE/test_global.h

PT_treepack.o: dlist.h
PT_treepack.o: globalprefs.h
PT_treepack.o: hooks.h
PT_treepack.o: pt_prototypes.h
PT_treepack.o: ptpan.h
PT_treepack.o: types.h
PT_treepack.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_treepack.o: $(ARBHOME)/INCLUDE/aisc_global.h
PT_treepack.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_treepack.o: $(ARBHOME)/INCLUDE/arb_core.h
PT_treepack.o: $(ARBHOME)/INCLUDE/arb_error.h
PT_treepack.o: $(ARBHOME)/INCLUDE/arb_msg.h
PT_treepack.o: $(ARBHOME)/INCLUDE/arb_string.h
PT_treepack.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_treepack.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PT_treepack.o: $(ARBHOME)/INCLUDE/arbtools.h
PT_treepack.o: $(ARBHOME)/INCLUDE/attributes.h
PT_treepack.o: $(ARBHOME)/INCLUDE/bytestring.h
PT_treepack.o: $(ARBHOME)/INCLUDE/dupstr.h
PT_treepack.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_treepack.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_treepack.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_treepack.o: $(ARBHOME)/INCLUDE/smartptr.h
PT_treepack.o: $(ARBHOME)/INCLUDE/test_global.h

PTP_buildtree.o: dlist.h
PTP_buildtree.o: globalprefs.h
PTP_buildtree.o: hooks.h
PTP_buildtree.o: pt_prototypes.h
PTP_buildtree.o: ptpan.h
PTP_buildtree.o: types.h
PTP_buildtree.o: $(ARBHOME)/INCLUDE/ad_prot.h
PTP_buildtree.o: $(ARBHOME)/INCLUDE/aisc_global.h
PTP_buildtree.o: $(ARBHOME)/INCLUDE/arb_assert.h
PTP_buildtree.o: $(ARBHOME)/INCLUDE/arb_core.h
PTP_buildtree.o: $(ARBHOME)/INCLUDE/arb_error.h
PTP_buildtree.o: $(ARBHOME)/INCLUDE/arb_msg.h
PTP_buildtree.o: $(ARBHOME)/INCLUDE/arb_string.h
PTP_buildtree.o: $(ARBHOME)/INCLUDE/arbdb.h
PTP_buildtree.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PTP_buildtree.o: $(ARBHOME)/INCLUDE/arbtools.h
PTP_buildtree.o: $(ARBHOME)/INCLUDE/attributes.h
PTP_buildtree.o: $(ARBHOME)/INCLUDE/bytestring.h
PTP_buildtree.o: $(ARBHOME)/INCLUDE/dupstr.h
PTP_buildtree.o: $(ARBHOME)/INCLUDE/PT_com.h
PTP_buildtree.o: $(ARBHOME)/INCLUDE/PT_server.h
PTP_buildtree.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PTP_buildtree.o: $(ARBHOME)/INCLUDE/smartptr.h
PTP_buildtree.o: $(ARBHOME)/INCLUDE/test_global.h

PTP_etc.o: dlist.h
PTP_etc.o: globalprefs.h
PTP_etc.o: hooks.h
PTP_etc.o: pt_prototypes.h
PTP_etc.o: ptpan.h
PTP_etc.o: types.h
PTP_etc.o: $(ARBHOME)/INCLUDE/ad_prot.h
PTP_etc.o: $(ARBHOME)/INCLUDE/aisc_global.h
PTP_etc.o: $(ARBHOME)/INCLUDE/arb_assert.h
PTP_etc.o: $(ARBHOME)/INCLUDE/arb_core.h
PTP_etc.o: $(ARBHOME)/INCLUDE/arb_error.h
PTP_etc.o: $(ARBHOME)/INCLUDE/arb_msg.h
PTP_etc.o: $(ARBHOME)/INCLUDE/arb_string.h
PTP_etc.o: $(ARBHOME)/INCLUDE/arbdb.h
PTP_etc.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PTP_etc.o: $(ARBHOME)/INCLUDE/arbtools.h
PTP_etc.o: $(ARBHOME)/INCLUDE/attributes.h
PTP_etc.o: $(ARBHOME)/INCLUDE/bytestring.h
PTP_etc.o: $(ARBHOME)/INCLUDE/dupstr.h
PTP_etc.o: $(ARBHOME)/INCLUDE/PT_com.h
PTP_etc.o: $(ARBHOME)/INCLUDE/PT_server.h
PTP_etc.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PTP_etc.o: $(ARBHOME)/INCLUDE/smartptr.h
PTP_etc.o: $(ARBHOME)/INCLUDE/struct_man.h
PTP_etc.o: $(ARBHOME)/INCLUDE/test_global.h

PTP_family.o: dlist.h
PTP_family.o: globalprefs.h
PTP_family.o: hooks.h
PTP_family.o: pt_prototypes.h
PTP_family.o: ptpan.h
PTP_family.o: types.h
PTP_family.o: $(ARBHOME)/INCLUDE/ad_prot.h
PTP_family.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PTP_family.o: $(ARBHOME)/INCLUDE/aisc_global.h
PTP_family.o: $(ARBHOME)/INCLUDE/arb_assert.h
PTP_family.o: $(ARBHOME)/INCLUDE/arb_core.h
PTP_family.o: $(ARBHOME)/INCLUDE/arb_error.h
PTP_family.o: $(ARBHOME)/INCLUDE/arb_msg.h
PTP_family.o: $(ARBHOME)/INCLUDE/arb_string.h
PTP_family.o: $(ARBHOME)/INCLUDE/arbdb.h
PTP_family.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PTP_family.o: $(ARBHOME)/INCLUDE/arbdbt.h
PTP_family.o: $(ARBHOME)/INCLUDE/arbtools.h
PTP_family.o: $(ARBHOME)/INCLUDE/attributes.h
PTP_family.o: $(ARBHOME)/INCLUDE/bytestring.h
PTP_family.o: $(ARBHOME)/INCLUDE/dupstr.h
PTP_family.o: $(ARBHOME)/INCLUDE/PT_com.h
PTP_family.o: $(ARBHOME)/INCLUDE/PT_server.h
PTP_family.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PTP_family.o: $(ARBHOME)/INCLUDE/smartptr.h
PTP_family.o: $(ARBHOME)/INCLUDE/struct_man.h
PTP_family.o: $(ARBHOME)/INCLUDE/test_global.h

PTP_findEx.o: dlist.h
PTP_findEx.o: globalprefs.h
PTP_findEx.o: hooks.h
PTP_findEx.o: pt_prototypes.h
PTP_findEx.o: ptpan.h
PTP_findEx.o: types.h
PTP_findEx.o: $(ARBHOME)/INCLUDE/ad_prot.h
PTP_findEx.o: $(ARBHOME)/INCLUDE/aisc_global.h
PTP_findEx.o: $(ARBHOME)/INCLUDE/arb_assert.h
PTP_findEx.o: $(ARBHOME)/INCLUDE/arb_core.h
PTP_findEx.o: $(ARBHOME)/INCLUDE/arb_error.h
PTP_findEx.o: $(ARBHOME)/INCLUDE/arb_msg.h
PTP_findEx.o: $(ARBHOME)/INCLUDE/arb_string.h
PTP_findEx.o: $(ARBHOME)/INCLUDE/arbdb.h
PTP_findEx.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PTP_findEx.o: $(ARBHOME)/INCLUDE/arbtools.h
PTP_findEx.o: $(ARBHOME)/INCLUDE/attributes.h
PTP_findEx.o: $(ARBHOME)/INCLUDE/bytestring.h
PTP_findEx.o: $(ARBHOME)/INCLUDE/dupstr.h
PTP_findEx.o: $(ARBHOME)/INCLUDE/PT_com.h
PTP_findEx.o: $(ARBHOME)/INCLUDE/PT_server.h
PTP_findEx.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PTP_findEx.o: $(ARBHOME)/INCLUDE/smartptr.h
PTP_findEx.o: $(ARBHOME)/INCLUDE/test_global.h

PTP_io.o: dlist.h
PTP_io.o: globalprefs.h
PTP_io.o: hooks.h
PTP_io.o: pt_prototypes.h
PTP_io.o: ptpan.h
PTP_io.o: types.h
PTP_io.o: $(ARBHOME)/INCLUDE/ad_prot.h
PTP_io.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PTP_io.o: $(ARBHOME)/INCLUDE/aisc_global.h
PTP_io.o: $(ARBHOME)/INCLUDE/arb_assert.h
PTP_io.o: $(ARBHOME)/INCLUDE/arb_core.h
PTP_io.o: $(ARBHOME)/INCLUDE/arb_error.h
PTP_io.o: $(ARBHOME)/INCLUDE/arb_msg.h
PTP_io.o: $(ARBHOME)/INCLUDE/arb_string.h
PTP_io.o: $(ARBHOME)/INCLUDE/arbdb.h
PTP_io.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PTP_io.o: $(ARBHOME)/INCLUDE/arbdbt.h
PTP_io.o: $(ARBHOME)/INCLUDE/arbtools.h
PTP_io.o: $(ARBHOME)/INCLUDE/attributes.h
PTP_io.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
PTP_io.o: $(ARBHOME)/INCLUDE/bytestring.h
PTP_io.o: $(ARBHOME)/INCLUDE/dupstr.h
PTP_io.o: $(ARBHOME)/INCLUDE/PT_com.h
PTP_io.o: $(ARBHOME)/INCLUDE/PT_server.h
PTP_io.o: $(ARBHOME)/INCLUDE/smartptr.h
PTP_io.o: $(ARBHOME)/INCLUDE/test_global.h

PTP_main.o: dlist.h
PTP_main.o: globalprefs.h
PTP_main.o: hooks.h
PTP_main.o: pt_prototypes.h
PTP_main.o: ptpan.h
PTP_main.o: types.h
PTP_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
PTP_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PTP_main.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
PTP_main.o: $(ARBHOME)/INCLUDE/aisc_global.h
PTP_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
PTP_main.o: $(ARBHOME)/INCLUDE/arb_core.h
PTP_main.o: $(ARBHOME)/INCLUDE/arb_error.h
PTP_main.o: $(ARBHOME)/INCLUDE/arb_msg.h
PTP_main.o: $(ARBHOME)/INCLUDE/arb_string.h
PTP_main.o: $(ARBHOME)/INCLUDE/arbdb.h
PTP_main.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PTP_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
PTP_main.o: $(ARBHOME)/INCLUDE/arbtools.h
PTP_main.o: $(ARBHOME)/INCLUDE/attributes.h
PTP_main.o: $(ARBHOME)/INCLUDE/bytestring.h
PTP_main.o: $(ARBHOME)/INCLUDE/client.h
PTP_main.o: $(ARBHOME)/INCLUDE/client_types.h
PTP_main.o: $(ARBHOME)/INCLUDE/dupstr.h
PTP_main.o: $(ARBHOME)/INCLUDE/PT_com.h
PTP_main.o: $(ARBHOME)/INCLUDE/PT_server.h
PTP_main.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PTP_main.o: $(ARBHOME)/INCLUDE/server.h
PTP_main.o: $(ARBHOME)/INCLUDE/servercntrl.h
PTP_main.o: $(ARBHOME)/INCLUDE/smartptr.h
PTP_main.o: $(ARBHOME)/INCLUDE/struct_man.h
PTP_main.o: $(ARBHOME)/INCLUDE/test_global.h

PTP_match.o: dlist.h
PTP_match.o: globalprefs.h
PTP_match.o: hooks.h
PTP_match.o: pt_prototypes.h
PTP_match.o: ptpan.h
PTP_match.o: types.h
PTP_match.o: $(ARBHOME)/INCLUDE/ad_prot.h
PTP_match.o: $(ARBHOME)/INCLUDE/aisc_global.h
PTP_match.o: $(ARBHOME)/INCLUDE/arb_assert.h
PTP_match.o: $(ARBHOME)/INCLUDE/arb_core.h
PTP_match.o: $(ARBHOME)/INCLUDE/arb_error.h
PTP_match.o: $(ARBHOME)/INCLUDE/arb_msg.h
PTP_match.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
PTP_match.o: $(ARBHOME)/INCLUDE/arb_string.h
PTP_match.o: $(ARBHOME)/INCLUDE/arbdb.h
PTP_match.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PTP_match.o: $(ARBHOME)/INCLUDE/arbtools.h
PTP_match.o: $(ARBHOME)/INCLUDE/attributes.h
PTP_match.o: $(ARBHOME)/INCLUDE/bytestring.h
PTP_match.o: $(ARBHOME)/INCLUDE/dupstr.h
PTP_match.o: $(ARBHOME)/INCLUDE/PT_com.h
PTP_match.o: $(ARBHOME)/INCLUDE/PT_server.h
PTP_match.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PTP_match.o: $(ARBHOME)/INCLUDE/smartptr.h
PTP_match.o: $(ARBHOME)/INCLUDE/struct_man.h
PTP_match.o: $(ARBHOME)/INCLUDE/test_global.h

PTP_new_design.o: dlist.h
PTP_new_design.o: globalprefs.h
PTP_new_design.o: hooks.h
PTP_new_design.o: pt_prototypes.h
PTP_new_design.o: ptpan.h
PTP_new_design.o: types.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/ad_prot.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/aisc_global.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/arb_assert.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/arb_core.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/arb_error.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/arb_msg.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/arb_string.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/arbdb.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/arbdbt.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/arbtools.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/attributes.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/bytestring.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/dupstr.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/PT_com.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/PT_server.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/smartptr.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/struct_man.h
PTP_new_design.o: $(ARBHOME)/INCLUDE/test_global.h
