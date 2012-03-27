# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

OBJECTS=$(subst .cxx,.o,$(wildcard *.cxx))

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES) $(POST_COMPILE)

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

ui_species.o: dbui.h
ui_species.o: $(ARBHOME)/INCLUDE/ad_prot.h
ui_species.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ui_species.o: $(ARBHOME)/INCLUDE/arb_assert.h
ui_species.o: $(ARBHOME)/INCLUDE/arb_core.h
ui_species.o: $(ARBHOME)/INCLUDE/arb_defs.h
ui_species.o: $(ARBHOME)/INCLUDE/arb_error.h
ui_species.o: $(ARBHOME)/INCLUDE/arb_msg.h
ui_species.o: $(ARBHOME)/INCLUDE/arb_progress.h
ui_species.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
ui_species.o: $(ARBHOME)/INCLUDE/arb_string.h
ui_species.o: $(ARBHOME)/INCLUDE/arbdb.h
ui_species.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ui_species.o: $(ARBHOME)/INCLUDE/arbdbt.h
ui_species.o: $(ARBHOME)/INCLUDE/arbtools.h
ui_species.o: $(ARBHOME)/INCLUDE/attributes.h
ui_species.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_base.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_detach.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_question.hxx
ui_species.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_select.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ui_species.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
ui_species.o: $(ARBHOME)/INCLUDE/awtc_next_neighbours.hxx
ui_species.o: $(ARBHOME)/INCLUDE/cb.h
ui_species.o: $(ARBHOME)/INCLUDE/db_query.h
ui_species.o: $(ARBHOME)/INCLUDE/db_scanner.hxx
ui_species.o: $(ARBHOME)/INCLUDE/dupstr.h
ui_species.o: $(ARBHOME)/INCLUDE/item_sel_list.h
ui_species.o: $(ARBHOME)/INCLUDE/items.h
ui_species.o: $(ARBHOME)/INCLUDE/probe_design.hxx
ui_species.o: $(ARBHOME)/INCLUDE/PT_global_defs.h
ui_species.o: $(ARBHOME)/INCLUDE/smartptr.h
ui_species.o: $(ARBHOME)/INCLUDE/test_global.h
