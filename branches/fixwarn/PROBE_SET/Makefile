# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt
.SUFFIXES: .o .cxx .depend

# --------------------------------------------------------------------------------

LIBS = $(LIBPATH) $(EXECLIBS) $(ARBDB_LIB) $(SYSLIBS)

# --------------------------------------------------------------------------------

DEST_EXECUTABLES = \
	$(ARBHOME)/PROBE_SET/bin/ps_show_result \
	$(ARBHOME)/PROBE_SET/bin/ps_arb2asciipaths \
	$(ARBHOME)/PROBE_SET/bin/ps_my2asciipaths \
	$(ARBHOME)/PROBE_SET/bin/ps_my2ascii \
	$(ARBHOME)/PROBE_SET/bin/ps_get_probes \
	$(ARBHOME)/PROBE_SET/bin/ps_eval \
	$(ARBHOME)/PROBE_SET/bin/ps_find \
	$(ARBHOME)/PROBE_SET/bin/ps_detect \
	$(ARBHOME)/PROBE_SET/bin/ps_merge \
	$(ARBHOME)/PROBE_SET/bin/ps_convert \
	$(ARBHOME)/PROBE_SET/bin/fb_test \

all : bin $(DEST_EXECUTABLES)

bin:
		mkdir -p bin

# --------------------------------------------------------------------------------

SHARED_OBJECTS = ps_node.o ps_filebuffer.o ps_database.o ps_tools.o

# --------------------------------------------------------------------------------

FB_TEST_OBJECTS = fb_test.o
$(ARBHOME)/PROBE_SET/bin/fb_test : $(SHARED_OBJECTS) $(FB_TEST_OBJECTS) $(use_ARB_main)
	$(LINK_EXECUTABLE) $@ $^ $(LIBS)

PS_CONVERT_OBJECTS = ps_convert_db.o
$(ARBHOME)/PROBE_SET/bin/ps_convert : $(SHARED_OBJECTS) $(PS_CONVERT_OBJECTS)
	$(LINK_EXECUTABLE) $@ $^ $(LIBS)

PS_MERGE_OBJECTS = ps_merge_my_dbs.o
$(ARBHOME)/PROBE_SET/bin/ps_merge : $(SHARED_OBJECTS) $(PS_MERGE_OBJECTS)
	$(LINK_EXECUTABLE) $@ $^ $(LIBS)

PS_DETECT_OBJECTS = ps_detect_weak_differences.o
$(ARBHOME)/PROBE_SET/bin/ps_detect : $(SHARED_OBJECTS) $(PS_DETECT_OBJECTS)
	$(LINK_EXECUTABLE) $@ $^ $(LIBS)

PS_FIND_OBJECTS = ps_find_probes.o
$(ARBHOME)/PROBE_SET/bin/ps_find : $(SHARED_OBJECTS) $(PS_FIND_OBJECTS)
	$(LINK_EXECUTABLE) $@ $^ $(LIBS)

PS_EVAL_OBJECTS = ps_eval_candidates.o
$(ARBHOME)/PROBE_SET/bin/ps_eval : $(SHARED_OBJECTS) $(PS_EVAL_OBJECTS)
	$(LINK_EXECUTABLE) $@ $^ $(LIBS)

PS_GET_PROBES_OBJECTS = ps_get_probes.o
$(ARBHOME)/PROBE_SET/bin/ps_get_probes : $(SHARED_OBJECTS) $(PS_GET_PROBES_OBJECTS)
	$(LINK_EXECUTABLE) $@ $^ $(LIBS)

PS_MY2ASCII_OBJECTS = ps_my2ascii.o
$(ARBHOME)/PROBE_SET/bin/ps_my2ascii : $(SHARED_OBJECTS) $(PS_MY2ASCII_OBJECTS)
	$(LINK_EXECUTABLE) $@ $^ $(LIBS)

PS_MY2ASCIIPATHS_OBJECTS = ps_my2asciipaths.o
$(ARBHOME)/PROBE_SET/bin/ps_my2asciipaths : $(SHARED_OBJECTS) $(PS_MY2ASCIIPATHS_OBJECTS)
	$(LINK_EXECUTABLE) $@ $^ $(LIBS)

PS_ARB2ASCIIPATHS_OBJECTS = ps_arb2asciipaths.o
$(ARBHOME)/PROBE_SET/bin/ps_arb2asciipaths : $(SHARED_OBJECTS) $(PS_ARB2ASCIIPATHS_OBJECTS)
	$(LINK_EXECUTABLE) $@ $^ $(LIBS)

PS_SHOW_RESULT_OBJECTS = ps_show_result.o
$(ARBHOME)/PROBE_SET/bin/ps_show_result : $(SHARED_OBJECTS) $(PS_SHOW_RESULT_OBJECTS)
	$(LINK_EXECUTABLE) $@ $^ $(LIBS)

CPP_OBJECTS = $(SHARED_OBJECTS) \
	$(FB_TEST_OBJECTS) \
	$(PS_CONVERT_OBJECTS) \
	$(PS_MERGE_OBJECTS) \
	$(PS_DETECT_OBJECTS) \
	$(PS_FIND_OBJECTS) \
	$(PS_EVAL_OBJECTS) \
	$(PS_GET_PROBES_OBJECTS) \
	$(PS_MY2ASCII_OBJECTS) \
	$(PS_MY2ASCIIPATHS_OBJECTS) \
	$(PS_ARB2ASCIIPATHS_OBJECTS) \
	$(PS_SHOW_RESULT_OBJECTS) \

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c -o $@ $(@:.o=.cxx)  $(CXX_INCLUDES) $(POST_COMPILE) $<

clean:
	rm -f $(CPP_OBJECTS)
	rm -f $(DEST_EXECUTABLES)

DEPENDS = $(CPP_OBJECTS:.o=.depend)
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

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

fb_test.o: ps_assert.hxx
fb_test.o: ps_bitmap.hxx
fb_test.o: ps_bitset.hxx
fb_test.o: ps_defs.hxx
fb_test.o: ps_filebuffer.hxx
fb_test.o: ps_node.hxx
fb_test.o: $(ARBHOME)/INCLUDE/arb_assert.h
fb_test.o: $(ARBHOME)/INCLUDE/arb_core.h
fb_test.o: $(ARBHOME)/INCLUDE/arb_file.h
fb_test.o: $(ARBHOME)/INCLUDE/arb_msg.h
fb_test.o: $(ARBHOME)/INCLUDE/arbtools.h
fb_test.o: $(ARBHOME)/INCLUDE/attributes.h
fb_test.o: $(ARBHOME)/INCLUDE/cxxforward.h
fb_test.o: $(ARBHOME)/INCLUDE/dupstr.h
fb_test.o: $(ARBHOME)/INCLUDE/gccver.h
fb_test.o: $(ARBHOME)/INCLUDE/smartptr.h
fb_test.o: $(ARBHOME)/INCLUDE/test_global.h
fb_test.o: $(ARBHOME)/INCLUDE/test_unit.h

ps_arb2asciipaths.o: ps_assert.hxx
ps_arb2asciipaths.o: ps_defs.hxx
ps_arb2asciipaths.o: ps_filebuffer.hxx
ps_arb2asciipaths.o: ps_pg_tree_functions.hxx
ps_arb2asciipaths.o: $(ARBHOME)/INCLUDE/ad_prot.h
ps_arb2asciipaths.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_arb2asciipaths.o: $(ARBHOME)/INCLUDE/arb_core.h
ps_arb2asciipaths.o: $(ARBHOME)/INCLUDE/arb_error.h
ps_arb2asciipaths.o: $(ARBHOME)/INCLUDE/arb_msg.h
ps_arb2asciipaths.o: $(ARBHOME)/INCLUDE/arb_string.h
ps_arb2asciipaths.o: $(ARBHOME)/INCLUDE/arbdb.h
ps_arb2asciipaths.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ps_arb2asciipaths.o: $(ARBHOME)/INCLUDE/arbtools.h
ps_arb2asciipaths.o: $(ARBHOME)/INCLUDE/attributes.h
ps_arb2asciipaths.o: $(ARBHOME)/INCLUDE/cxxforward.h
ps_arb2asciipaths.o: $(ARBHOME)/INCLUDE/dupstr.h
ps_arb2asciipaths.o: $(ARBHOME)/INCLUDE/gccver.h
ps_arb2asciipaths.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_arb2asciipaths.o: $(ARBHOME)/INCLUDE/test_global.h

ps_convert_db.o: ps_assert.hxx
ps_convert_db.o: ps_database.hxx
ps_convert_db.o: ps_defs.hxx
ps_convert_db.o: ps_filebuffer.hxx
ps_convert_db.o: ps_node.hxx
ps_convert_db.o: ps_pg_specmap.hxx
ps_convert_db.o: ps_pg_tree_functions.hxx
ps_convert_db.o: ps_tools.hxx
ps_convert_db.o: $(ARBHOME)/INCLUDE/ad_prot.h
ps_convert_db.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_convert_db.o: $(ARBHOME)/INCLUDE/arb_core.h
ps_convert_db.o: $(ARBHOME)/INCLUDE/arb_error.h
ps_convert_db.o: $(ARBHOME)/INCLUDE/arb_msg.h
ps_convert_db.o: $(ARBHOME)/INCLUDE/arb_string.h
ps_convert_db.o: $(ARBHOME)/INCLUDE/arbdb.h
ps_convert_db.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ps_convert_db.o: $(ARBHOME)/INCLUDE/arbtools.h
ps_convert_db.o: $(ARBHOME)/INCLUDE/attributes.h
ps_convert_db.o: $(ARBHOME)/INCLUDE/cxxforward.h
ps_convert_db.o: $(ARBHOME)/INCLUDE/dupstr.h
ps_convert_db.o: $(ARBHOME)/INCLUDE/gccver.h
ps_convert_db.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_convert_db.o: $(ARBHOME)/INCLUDE/test_global.h

ps_database.o: ps_assert.hxx
ps_database.o: ps_database.hxx
ps_database.o: ps_defs.hxx
ps_database.o: ps_filebuffer.hxx
ps_database.o: ps_node.hxx
ps_database.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_database.o: $(ARBHOME)/INCLUDE/arb_core.h
ps_database.o: $(ARBHOME)/INCLUDE/arbtools.h
ps_database.o: $(ARBHOME)/INCLUDE/attributes.h
ps_database.o: $(ARBHOME)/INCLUDE/cxxforward.h
ps_database.o: $(ARBHOME)/INCLUDE/dupstr.h
ps_database.o: $(ARBHOME)/INCLUDE/gccver.h
ps_database.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_database.o: $(ARBHOME)/INCLUDE/test_global.h

ps_detect_weak_differences.o: ps_assert.hxx
ps_detect_weak_differences.o: ps_bitmap.hxx
ps_detect_weak_differences.o: ps_bitset.hxx
ps_detect_weak_differences.o: ps_database.hxx
ps_detect_weak_differences.o: ps_defs.hxx
ps_detect_weak_differences.o: ps_filebuffer.hxx
ps_detect_weak_differences.o: ps_node.hxx
ps_detect_weak_differences.o: ps_tools.hxx
ps_detect_weak_differences.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_detect_weak_differences.o: $(ARBHOME)/INCLUDE/arb_core.h
ps_detect_weak_differences.o: $(ARBHOME)/INCLUDE/arbtools.h
ps_detect_weak_differences.o: $(ARBHOME)/INCLUDE/attributes.h
ps_detect_weak_differences.o: $(ARBHOME)/INCLUDE/cxxforward.h
ps_detect_weak_differences.o: $(ARBHOME)/INCLUDE/dupstr.h
ps_detect_weak_differences.o: $(ARBHOME)/INCLUDE/gccver.h
ps_detect_weak_differences.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_detect_weak_differences.o: $(ARBHOME)/INCLUDE/test_global.h

ps_eval_candidates.o: ps_assert.hxx
ps_eval_candidates.o: ps_bitmap.hxx
ps_eval_candidates.o: ps_bitset.hxx
ps_eval_candidates.o: ps_candidate.hxx
ps_eval_candidates.o: ps_database.hxx
ps_eval_candidates.o: ps_defs.hxx
ps_eval_candidates.o: ps_filebuffer.hxx
ps_eval_candidates.o: ps_node.hxx
ps_eval_candidates.o: ps_tools.hxx
ps_eval_candidates.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_eval_candidates.o: $(ARBHOME)/INCLUDE/arb_core.h
ps_eval_candidates.o: $(ARBHOME)/INCLUDE/arbtools.h
ps_eval_candidates.o: $(ARBHOME)/INCLUDE/attributes.h
ps_eval_candidates.o: $(ARBHOME)/INCLUDE/cxxforward.h
ps_eval_candidates.o: $(ARBHOME)/INCLUDE/dupstr.h
ps_eval_candidates.o: $(ARBHOME)/INCLUDE/gccver.h
ps_eval_candidates.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_eval_candidates.o: $(ARBHOME)/INCLUDE/test_global.h

ps_filebuffer.o: ps_assert.hxx
ps_filebuffer.o: ps_filebuffer.hxx
ps_filebuffer.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_filebuffer.o: $(ARBHOME)/INCLUDE/arb_core.h
ps_filebuffer.o: $(ARBHOME)/INCLUDE/arbtools.h
ps_filebuffer.o: $(ARBHOME)/INCLUDE/attributes.h
ps_filebuffer.o: $(ARBHOME)/INCLUDE/cxxforward.h
ps_filebuffer.o: $(ARBHOME)/INCLUDE/dupstr.h
ps_filebuffer.o: $(ARBHOME)/INCLUDE/gccver.h
ps_filebuffer.o: $(ARBHOME)/INCLUDE/test_global.h

ps_find_probes.o: ps_assert.hxx
ps_find_probes.o: ps_bitmap.hxx
ps_find_probes.o: ps_bitset.hxx
ps_find_probes.o: ps_candidate.hxx
ps_find_probes.o: ps_database.hxx
ps_find_probes.o: ps_defs.hxx
ps_find_probes.o: ps_filebuffer.hxx
ps_find_probes.o: ps_node.hxx
ps_find_probes.o: ps_tools.hxx
ps_find_probes.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_find_probes.o: $(ARBHOME)/INCLUDE/arb_core.h
ps_find_probes.o: $(ARBHOME)/INCLUDE/arbtools.h
ps_find_probes.o: $(ARBHOME)/INCLUDE/attributes.h
ps_find_probes.o: $(ARBHOME)/INCLUDE/cxxforward.h
ps_find_probes.o: $(ARBHOME)/INCLUDE/dupstr.h
ps_find_probes.o: $(ARBHOME)/INCLUDE/gccver.h
ps_find_probes.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_find_probes.o: $(ARBHOME)/INCLUDE/test_global.h

ps_get_probes.o: ps_assert.hxx
ps_get_probes.o: ps_defs.hxx
ps_get_probes.o: ps_filebuffer.hxx
ps_get_probes.o: ps_pg_tree_functions.hxx
ps_get_probes.o: $(ARBHOME)/INCLUDE/ad_prot.h
ps_get_probes.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_get_probes.o: $(ARBHOME)/INCLUDE/arb_core.h
ps_get_probes.o: $(ARBHOME)/INCLUDE/arb_error.h
ps_get_probes.o: $(ARBHOME)/INCLUDE/arb_msg.h
ps_get_probes.o: $(ARBHOME)/INCLUDE/arb_string.h
ps_get_probes.o: $(ARBHOME)/INCLUDE/arbdb.h
ps_get_probes.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ps_get_probes.o: $(ARBHOME)/INCLUDE/arbtools.h
ps_get_probes.o: $(ARBHOME)/INCLUDE/attributes.h
ps_get_probes.o: $(ARBHOME)/INCLUDE/cxxforward.h
ps_get_probes.o: $(ARBHOME)/INCLUDE/dupstr.h
ps_get_probes.o: $(ARBHOME)/INCLUDE/gccver.h
ps_get_probes.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_get_probes.o: $(ARBHOME)/INCLUDE/test_global.h

ps_merge_my_dbs.o: ps_assert.hxx
ps_merge_my_dbs.o: ps_database.hxx
ps_merge_my_dbs.o: ps_defs.hxx
ps_merge_my_dbs.o: ps_filebuffer.hxx
ps_merge_my_dbs.o: ps_node.hxx
ps_merge_my_dbs.o: ps_tools.hxx
ps_merge_my_dbs.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_merge_my_dbs.o: $(ARBHOME)/INCLUDE/arb_core.h
ps_merge_my_dbs.o: $(ARBHOME)/INCLUDE/arbtools.h
ps_merge_my_dbs.o: $(ARBHOME)/INCLUDE/attributes.h
ps_merge_my_dbs.o: $(ARBHOME)/INCLUDE/cxxforward.h
ps_merge_my_dbs.o: $(ARBHOME)/INCLUDE/dupstr.h
ps_merge_my_dbs.o: $(ARBHOME)/INCLUDE/gccver.h
ps_merge_my_dbs.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_merge_my_dbs.o: $(ARBHOME)/INCLUDE/test_global.h

ps_my2ascii.o: ps_assert.hxx
ps_my2ascii.o: ps_defs.hxx
ps_my2ascii.o: ps_filebuffer.hxx
ps_my2ascii.o: ps_node.hxx
ps_my2ascii.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_my2ascii.o: $(ARBHOME)/INCLUDE/arb_core.h
ps_my2ascii.o: $(ARBHOME)/INCLUDE/arbtools.h
ps_my2ascii.o: $(ARBHOME)/INCLUDE/attributes.h
ps_my2ascii.o: $(ARBHOME)/INCLUDE/cxxforward.h
ps_my2ascii.o: $(ARBHOME)/INCLUDE/dupstr.h
ps_my2ascii.o: $(ARBHOME)/INCLUDE/gccver.h
ps_my2ascii.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_my2ascii.o: $(ARBHOME)/INCLUDE/test_global.h

ps_my2asciipaths.o: ps_assert.hxx
ps_my2asciipaths.o: ps_defs.hxx
ps_my2asciipaths.o: ps_filebuffer.hxx
ps_my2asciipaths.o: ps_node.hxx
ps_my2asciipaths.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_my2asciipaths.o: $(ARBHOME)/INCLUDE/arb_core.h
ps_my2asciipaths.o: $(ARBHOME)/INCLUDE/arbtools.h
ps_my2asciipaths.o: $(ARBHOME)/INCLUDE/attributes.h
ps_my2asciipaths.o: $(ARBHOME)/INCLUDE/cxxforward.h
ps_my2asciipaths.o: $(ARBHOME)/INCLUDE/dupstr.h
ps_my2asciipaths.o: $(ARBHOME)/INCLUDE/gccver.h
ps_my2asciipaths.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_my2asciipaths.o: $(ARBHOME)/INCLUDE/test_global.h

ps_node.o: ps_assert.hxx
ps_node.o: ps_defs.hxx
ps_node.o: ps_filebuffer.hxx
ps_node.o: ps_node.hxx
ps_node.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_node.o: $(ARBHOME)/INCLUDE/arb_core.h
ps_node.o: $(ARBHOME)/INCLUDE/arbtools.h
ps_node.o: $(ARBHOME)/INCLUDE/attributes.h
ps_node.o: $(ARBHOME)/INCLUDE/cxxforward.h
ps_node.o: $(ARBHOME)/INCLUDE/dupstr.h
ps_node.o: $(ARBHOME)/INCLUDE/gccver.h
ps_node.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_node.o: $(ARBHOME)/INCLUDE/test_global.h

ps_show_result.o: ps_assert.hxx
ps_show_result.o: ps_bitmap.hxx
ps_show_result.o: ps_bitset.hxx
ps_show_result.o: ps_defs.hxx
ps_show_result.o: ps_filebuffer.hxx
ps_show_result.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_show_result.o: $(ARBHOME)/INCLUDE/arb_core.h
ps_show_result.o: $(ARBHOME)/INCLUDE/arbtools.h
ps_show_result.o: $(ARBHOME)/INCLUDE/attributes.h
ps_show_result.o: $(ARBHOME)/INCLUDE/cxxforward.h
ps_show_result.o: $(ARBHOME)/INCLUDE/dupstr.h
ps_show_result.o: $(ARBHOME)/INCLUDE/gccver.h
ps_show_result.o: $(ARBHOME)/INCLUDE/test_global.h

ps_tools.o: ps_tools.hxx
