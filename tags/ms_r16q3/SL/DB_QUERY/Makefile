# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .cxx .depend

OBJECTS=$(subst .cxx,.o,$(wildcard *.cxx))

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

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

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from SL)

db_query.o: db_query.h
db_query.o: db_query_local.h
db_query.o: $(ARBHOME)/INCLUDE/ad_cb.h
db_query.o: $(ARBHOME)/INCLUDE/ad_cb_prot.h
db_query.o: $(ARBHOME)/INCLUDE/ad_colorset.h
db_query.o: $(ARBHOME)/INCLUDE/ad_prot.h
db_query.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
db_query.o: $(ARBHOME)/INCLUDE/arb_assert.h
db_query.o: $(ARBHOME)/INCLUDE/arb_core.h
db_query.o: $(ARBHOME)/INCLUDE/arb_error.h
db_query.o: $(ARBHOME)/INCLUDE/arb_global_defs.h
db_query.o: $(ARBHOME)/INCLUDE/arb_match.h
db_query.o: $(ARBHOME)/INCLUDE/arb_mem.h
db_query.o: $(ARBHOME)/INCLUDE/arb_msg.h
db_query.o: $(ARBHOME)/INCLUDE/arb_progress.h
db_query.o: $(ARBHOME)/INCLUDE/arb_sort.h
db_query.o: $(ARBHOME)/INCLUDE/arb_str.h
db_query.o: $(ARBHOME)/INCLUDE/arb_strarray.h
db_query.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
db_query.o: $(ARBHOME)/INCLUDE/arb_string.h
db_query.o: $(ARBHOME)/INCLUDE/arbdb.h
db_query.o: $(ARBHOME)/INCLUDE/arbdb_base.h
db_query.o: $(ARBHOME)/INCLUDE/arbdbt.h
db_query.o: $(ARBHOME)/INCLUDE/arbtools.h
db_query.o: $(ARBHOME)/INCLUDE/attributes.h
db_query.o: $(ARBHOME)/INCLUDE/aw_advice.hxx
db_query.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
db_query.o: $(ARBHOME)/INCLUDE/aw_base.hxx
db_query.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
db_query.o: $(ARBHOME)/INCLUDE/aw_file.hxx
db_query.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
db_query.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
db_query.o: $(ARBHOME)/INCLUDE/aw_question.hxx
db_query.o: $(ARBHOME)/INCLUDE/aw_root.hxx
db_query.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
db_query.o: $(ARBHOME)/INCLUDE/aw_select.hxx
db_query.o: $(ARBHOME)/INCLUDE/aw_window.hxx
db_query.o: $(ARBHOME)/INCLUDE/awt_config_manager.hxx
db_query.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
db_query.o: $(ARBHOME)/INCLUDE/cb.h
db_query.o: $(ARBHOME)/INCLUDE/cb_base.h
db_query.o: $(ARBHOME)/INCLUDE/cbtypes.h
db_query.o: $(ARBHOME)/INCLUDE/cxxforward.h
db_query.o: $(ARBHOME)/INCLUDE/dbui.h
db_query.o: $(ARBHOME)/INCLUDE/downcast.h
db_query.o: $(ARBHOME)/INCLUDE/dupstr.h
db_query.o: $(ARBHOME)/INCLUDE/gccver.h
db_query.o: $(ARBHOME)/INCLUDE/item_sel_list.h
db_query.o: $(ARBHOME)/INCLUDE/items.h
db_query.o: $(ARBHOME)/INCLUDE/Keeper.h
db_query.o: $(ARBHOME)/INCLUDE/rootAsWin.h
db_query.o: $(ARBHOME)/INCLUDE/smartptr.h
db_query.o: $(ARBHOME)/INCLUDE/static_assert.h
db_query.o: $(ARBHOME)/INCLUDE/test_global.h
db_query.o: $(ARBHOME)/INCLUDE/test_unit.h
db_query.o: $(ARBHOME)/INCLUDE/ttypes.h
