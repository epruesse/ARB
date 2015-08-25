.SUFFIXES: .o .cxx .depend

OBJECTS = $(subst .cxx,.o,$(wildcard *.cxx))
SERVERLIB=../PROBE_COM/server.a

# ---------------------------------------- allow misaligned data access in PROBE
PROBE_CFLAGS := $(cflags)
ifeq ('$(USE_GCC_50_OR_HIGHER)', 'yes')
 ifneq ('$(SANITIZE)', '0')
  PROBE_CFLAGS := $(cflags) -fno-sanitize=alignment
 endif
endif
# ---------------------------------------- 

$(MAIN): $(OBJECTS) $(SERVERLIB)
# $(SERVERLIB) and $(MAIN) reference each other
# unpack all objects from $(SERVERLIB) and ar them together with local objects
	mkdir -p serverlib
	cd serverlib;ar x ../$(SERVERLIB)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS) serverlib/*.o
	rm serverlib/*.o
ifeq ($(DARWIN),1)
	-rm serverlib/__.*
endif
	rmdir serverlib

.cxx.o:
	$(A_CXX) $(PROBE_CFLAGS) $(cxxflags) -c $<  $(CXX_INCLUDES) $(POST_COMPILE) $<

proto:
	../AISC_MKPTPS/aisc_mkpt -P -G -E -w pt_prototypes.h $(OBJECTS:.o=.cxx) probe_tree.h >pt_prototypes.h.tmp
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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

PT_buildtree.o: probe.h
PT_buildtree.o: probe_tree.h
PT_buildtree.o: PT_compress.h
PT_buildtree.o: PT_partition.h
PT_buildtree.o: PT_prefixIter.h
PT_buildtree.o: pt_prototypes.h
PT_buildtree.o: PT_tools.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/aisc_global.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arb_core.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arb_defs.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arb_diff.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arb_error.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arb_file.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arb_misc.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arb_msg.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arb_progress.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arb_string.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arbtools.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/attributes.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/bytestring.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/cache.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/cxxforward.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/dupstr.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/gccver.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/smartptr.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/static_assert.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/test_global.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/test_unit.h

PT_debug.o: probe.h
PT_debug.o: probe_tree.h
PT_debug.o: pt_prototypes.h
PT_debug.o: PT_tools.h
PT_debug.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_debug.o: $(ARBHOME)/INCLUDE/aisc_global.h
PT_debug.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_debug.o: $(ARBHOME)/INCLUDE/arb_core.h
PT_debug.o: $(ARBHOME)/INCLUDE/arb_error.h
PT_debug.o: $(ARBHOME)/INCLUDE/arb_file.h
PT_debug.o: $(ARBHOME)/INCLUDE/arb_misc.h
PT_debug.o: $(ARBHOME)/INCLUDE/arb_msg.h
PT_debug.o: $(ARBHOME)/INCLUDE/arb_string.h
PT_debug.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_debug.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PT_debug.o: $(ARBHOME)/INCLUDE/arbtools.h
PT_debug.o: $(ARBHOME)/INCLUDE/attributes.h
PT_debug.o: $(ARBHOME)/INCLUDE/bytestring.h
PT_debug.o: $(ARBHOME)/INCLUDE/cache.h
PT_debug.o: $(ARBHOME)/INCLUDE/cxxforward.h
PT_debug.o: $(ARBHOME)/INCLUDE/dupstr.h
PT_debug.o: $(ARBHOME)/INCLUDE/gccver.h
PT_debug.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_debug.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_debug.o: $(ARBHOME)/INCLUDE/smartptr.h
PT_debug.o: $(ARBHOME)/INCLUDE/static_assert.h
PT_debug.o: $(ARBHOME)/INCLUDE/test_global.h

PT_etc.o: probe.h
PT_etc.o: pt_prototypes.h
PT_etc.o: PT_tools.h
PT_etc.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_etc.o: $(ARBHOME)/INCLUDE/aisc_global.h
PT_etc.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_etc.o: $(ARBHOME)/INCLUDE/arb_core.h
PT_etc.o: $(ARBHOME)/INCLUDE/arb_error.h
PT_etc.o: $(ARBHOME)/INCLUDE/arb_msg.h
PT_etc.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
PT_etc.o: $(ARBHOME)/INCLUDE/arb_string.h
PT_etc.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_etc.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PT_etc.o: $(ARBHOME)/INCLUDE/arbtools.h
PT_etc.o: $(ARBHOME)/INCLUDE/attributes.h
PT_etc.o: $(ARBHOME)/INCLUDE/bytestring.h
PT_etc.o: $(ARBHOME)/INCLUDE/cache.h
PT_etc.o: $(ARBHOME)/INCLUDE/cxxforward.h
PT_etc.o: $(ARBHOME)/INCLUDE/dupstr.h
PT_etc.o: $(ARBHOME)/INCLUDE/gccver.h
PT_etc.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_etc.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_etc.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_etc.o: $(ARBHOME)/INCLUDE/smartptr.h
PT_etc.o: $(ARBHOME)/INCLUDE/static_assert.h
PT_etc.o: $(ARBHOME)/INCLUDE/struct_man.h
PT_etc.o: $(ARBHOME)/INCLUDE/test_global.h

PT_family.o: probe.h
PT_family.o: probe_tree.h
PT_family.o: PT_complement.h
PT_family.o: PT_global_defs.h
PT_family.o: pt_prototypes.h
PT_family.o: PT_rangeCheck.h
PT_family.o: PT_tools.h
PT_family.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_family.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_family.o: $(ARBHOME)/INCLUDE/aisc_global.h
PT_family.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_family.o: $(ARBHOME)/INCLUDE/arb_core.h
PT_family.o: $(ARBHOME)/INCLUDE/arb_error.h
PT_family.o: $(ARBHOME)/INCLUDE/arb_msg.h
PT_family.o: $(ARBHOME)/INCLUDE/arb_string.h
PT_family.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_family.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PT_family.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_family.o: $(ARBHOME)/INCLUDE/arbtools.h
PT_family.o: $(ARBHOME)/INCLUDE/attributes.h
PT_family.o: $(ARBHOME)/INCLUDE/bytestring.h
PT_family.o: $(ARBHOME)/INCLUDE/cache.h
PT_family.o: $(ARBHOME)/INCLUDE/cxxforward.h
PT_family.o: $(ARBHOME)/INCLUDE/downcast.h
PT_family.o: $(ARBHOME)/INCLUDE/dupstr.h
PT_family.o: $(ARBHOME)/INCLUDE/gccver.h
PT_family.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_family.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_family.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_family.o: $(ARBHOME)/INCLUDE/smartptr.h
PT_family.o: $(ARBHOME)/INCLUDE/static_assert.h
PT_family.o: $(ARBHOME)/INCLUDE/struct_man.h
PT_family.o: $(ARBHOME)/INCLUDE/test_global.h

PT_findEx.o: probe.h
PT_findEx.o: probe_tree.h
PT_findEx.o: pt_prototypes.h
PT_findEx.o: PT_tools.h
PT_findEx.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_findEx.o: $(ARBHOME)/INCLUDE/aisc_global.h
PT_findEx.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_findEx.o: $(ARBHOME)/INCLUDE/arb_core.h
PT_findEx.o: $(ARBHOME)/INCLUDE/arb_error.h
PT_findEx.o: $(ARBHOME)/INCLUDE/arb_msg.h
PT_findEx.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
PT_findEx.o: $(ARBHOME)/INCLUDE/arb_string.h
PT_findEx.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_findEx.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PT_findEx.o: $(ARBHOME)/INCLUDE/arbtools.h
PT_findEx.o: $(ARBHOME)/INCLUDE/attributes.h
PT_findEx.o: $(ARBHOME)/INCLUDE/bytestring.h
PT_findEx.o: $(ARBHOME)/INCLUDE/cache.h
PT_findEx.o: $(ARBHOME)/INCLUDE/cxxforward.h
PT_findEx.o: $(ARBHOME)/INCLUDE/dupstr.h
PT_findEx.o: $(ARBHOME)/INCLUDE/gccver.h
PT_findEx.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_findEx.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_findEx.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_findEx.o: $(ARBHOME)/INCLUDE/smartptr.h
PT_findEx.o: $(ARBHOME)/INCLUDE/static_assert.h
PT_findEx.o: $(ARBHOME)/INCLUDE/test_global.h

PT_io.o: probe.h
PT_io.o: PT_compress.h
PT_io.o: pt_prototypes.h
PT_io.o: PT_tools.h
PT_io.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_io.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_io.o: $(ARBHOME)/INCLUDE/aisc_global.h
PT_io.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_io.o: $(ARBHOME)/INCLUDE/arb_core.h
PT_io.o: $(ARBHOME)/INCLUDE/arb_error.h
PT_io.o: $(ARBHOME)/INCLUDE/arb_file.h
PT_io.o: $(ARBHOME)/INCLUDE/arb_msg.h
PT_io.o: $(ARBHOME)/INCLUDE/arb_progress.h
PT_io.o: $(ARBHOME)/INCLUDE/arb_string.h
PT_io.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_io.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PT_io.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_io.o: $(ARBHOME)/INCLUDE/arbtools.h
PT_io.o: $(ARBHOME)/INCLUDE/attributes.h
PT_io.o: $(ARBHOME)/INCLUDE/BI_basepos.hxx
PT_io.o: $(ARBHOME)/INCLUDE/bytestring.h
PT_io.o: $(ARBHOME)/INCLUDE/cache.h
PT_io.o: $(ARBHOME)/INCLUDE/cxxforward.h
PT_io.o: $(ARBHOME)/INCLUDE/downcast.h
PT_io.o: $(ARBHOME)/INCLUDE/dupstr.h
PT_io.o: $(ARBHOME)/INCLUDE/gccver.h
PT_io.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_io.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_io.o: $(ARBHOME)/INCLUDE/smartptr.h
PT_io.o: $(ARBHOME)/INCLUDE/static_assert.h
PT_io.o: $(ARBHOME)/INCLUDE/test_global.h
PT_io.o: $(ARBHOME)/INCLUDE/test_unit.h

PT_main.o: probe.h
PT_main.o: PT_mem.h
PT_main.o: pt_prototypes.h
PT_main.o: PT_tools.h
PT_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_main.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
PT_main.o: $(ARBHOME)/INCLUDE/aisc_global.h
PT_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_main.o: $(ARBHOME)/INCLUDE/arb_core.h
PT_main.o: $(ARBHOME)/INCLUDE/arb_defs.h
PT_main.o: $(ARBHOME)/INCLUDE/arb_error.h
PT_main.o: $(ARBHOME)/INCLUDE/arb_file.h
PT_main.o: $(ARBHOME)/INCLUDE/arb_misc.h
PT_main.o: $(ARBHOME)/INCLUDE/arb_msg.h
PT_main.o: $(ARBHOME)/INCLUDE/arb_sleep.h
PT_main.o: $(ARBHOME)/INCLUDE/arb_string.h
PT_main.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_main.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PT_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_main.o: $(ARBHOME)/INCLUDE/arbtools.h
PT_main.o: $(ARBHOME)/INCLUDE/attributes.h
PT_main.o: $(ARBHOME)/INCLUDE/BI_basepos.hxx
PT_main.o: $(ARBHOME)/INCLUDE/bytestring.h
PT_main.o: $(ARBHOME)/INCLUDE/cache.h
PT_main.o: $(ARBHOME)/INCLUDE/client.h
PT_main.o: $(ARBHOME)/INCLUDE/client_types.h
PT_main.o: $(ARBHOME)/INCLUDE/cxxforward.h
PT_main.o: $(ARBHOME)/INCLUDE/downcast.h
PT_main.o: $(ARBHOME)/INCLUDE/dupstr.h
PT_main.o: $(ARBHOME)/INCLUDE/gccver.h
PT_main.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_main.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_main.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_main.o: $(ARBHOME)/INCLUDE/ptclean.h
PT_main.o: $(ARBHOME)/INCLUDE/server.h
PT_main.o: $(ARBHOME)/INCLUDE/servercntrl.h
PT_main.o: $(ARBHOME)/INCLUDE/smartptr.h
PT_main.o: $(ARBHOME)/INCLUDE/static_assert.h
PT_main.o: $(ARBHOME)/INCLUDE/struct_man.h
PT_main.o: $(ARBHOME)/INCLUDE/test_global.h
PT_main.o: $(ARBHOME)/INCLUDE/ut_valgrinded.h

PT_match.o: probe.h
PT_match.o: probe_tree.h
PT_match.o: PT_complement.h
PT_match.o: pt_prototypes.h
PT_match.o: pt_split.h
PT_match.o: PT_tools.h
PT_match.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_match.o: $(ARBHOME)/INCLUDE/aisc_global.h
PT_match.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_match.o: $(ARBHOME)/INCLUDE/arb_core.h
PT_match.o: $(ARBHOME)/INCLUDE/arb_defs.h
PT_match.o: $(ARBHOME)/INCLUDE/arb_error.h
PT_match.o: $(ARBHOME)/INCLUDE/arb_msg.h
PT_match.o: $(ARBHOME)/INCLUDE/arb_sort.h
PT_match.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
PT_match.o: $(ARBHOME)/INCLUDE/arb_string.h
PT_match.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_match.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PT_match.o: $(ARBHOME)/INCLUDE/arbtools.h
PT_match.o: $(ARBHOME)/INCLUDE/attributes.h
PT_match.o: $(ARBHOME)/INCLUDE/bytestring.h
PT_match.o: $(ARBHOME)/INCLUDE/cache.h
PT_match.o: $(ARBHOME)/INCLUDE/cxxforward.h
PT_match.o: $(ARBHOME)/INCLUDE/dupstr.h
PT_match.o: $(ARBHOME)/INCLUDE/gccver.h
PT_match.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_match.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_match.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_match.o: $(ARBHOME)/INCLUDE/smartptr.h
PT_match.o: $(ARBHOME)/INCLUDE/static_assert.h
PT_match.o: $(ARBHOME)/INCLUDE/struct_man.h
PT_match.o: $(ARBHOME)/INCLUDE/test_global.h
PT_match.o: $(ARBHOME)/INCLUDE/test_unit.h

PT_new_design.o: probe.h
PT_new_design.o: probe_tree.h
PT_new_design.o: PT_complement.h
PT_new_design.o: PT_prefixIter.h
PT_new_design.o: pt_prototypes.h
PT_new_design.o: pt_split.h
PT_new_design.o: PT_tools.h
PT_new_design.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_new_design.o: $(ARBHOME)/INCLUDE/aisc_global.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arb_core.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arb_defs.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arb_error.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arb_msg.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arb_progress.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arb_sort.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arb_str.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arb_strarray.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arb_string.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arbtools.h
PT_new_design.o: $(ARBHOME)/INCLUDE/attributes.h
PT_new_design.o: $(ARBHOME)/INCLUDE/bytestring.h
PT_new_design.o: $(ARBHOME)/INCLUDE/cache.h
PT_new_design.o: $(ARBHOME)/INCLUDE/cxxforward.h
PT_new_design.o: $(ARBHOME)/INCLUDE/dupstr.h
PT_new_design.o: $(ARBHOME)/INCLUDE/gccver.h
PT_new_design.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_new_design.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_new_design.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_new_design.o: $(ARBHOME)/INCLUDE/smartptr.h
PT_new_design.o: $(ARBHOME)/INCLUDE/static_assert.h
PT_new_design.o: $(ARBHOME)/INCLUDE/struct_man.h
PT_new_design.o: $(ARBHOME)/INCLUDE/test_global.h
PT_new_design.o: $(ARBHOME)/INCLUDE/test_unit.h

PT_prefixtree.o: probe.h
PT_prefixtree.o: probe_tree.h
PT_prefixtree.o: PT_mem.h
PT_prefixtree.o: pt_prototypes.h
PT_prefixtree.o: PT_tools.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/aisc_global.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/arb_core.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/arb_error.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/arb_file.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/arb_misc.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/arb_msg.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/arb_string.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/arbtools.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/attributes.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/bytestring.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/cache.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/cxxforward.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/dupstr.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/gccver.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/smartptr.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/static_assert.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/test_global.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/test_unit.h

PT_rangeCheck.o: probe.h
PT_rangeCheck.o: probe_tree.h
PT_rangeCheck.o: pt_prototypes.h
PT_rangeCheck.o: PT_rangeCheck.h
PT_rangeCheck.o: PT_tools.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/aisc_global.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/arb_assert.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/arb_core.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/arb_error.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/arb_msg.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/arb_string.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/arbtools.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/attributes.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/bytestring.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/cache.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/cxxforward.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/dupstr.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/gccver.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/smartptr.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/static_assert.h
PT_rangeCheck.o: $(ARBHOME)/INCLUDE/test_global.h
