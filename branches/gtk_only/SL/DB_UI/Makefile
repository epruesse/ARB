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
ui_species.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_at.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_base.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_common.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_common_gtk.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_detach.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_help.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_position.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_question.hxx
ui_species.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_root_gtk.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_select.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_xfig.hxx
ui_species.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
ui_species.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
ui_species.o: $(ARBHOME)/INCLUDE/awtc_next_neighbours.hxx
ui_species.o: $(ARBHOME)/INCLUDE/cb.h
ui_species.o: $(ARBHOME)/INCLUDE/cxxforward.h
ui_species.o: $(ARBHOME)/INCLUDE/db_query.h
ui_species.o: $(ARBHOME)/INCLUDE/db_scanner.hxx
ui_species.o: $(ARBHOME)/INCLUDE/downcast.h
ui_species.o: $(ARBHOME)/INCLUDE/dupstr.h
ui_species.o: $(ARBHOME)/INCLUDE/gccver.h
ui_species.o: $(ARBHOME)/INCLUDE/item_sel_list.h
ui_species.o: $(ARBHOME)/INCLUDE/items.h
ui_species.o: $(ARBHOME)/INCLUDE/pos_range.h
ui_species.o: $(ARBHOME)/INCLUDE/probe_design.hxx
ui_species.o: $(ARBHOME)/INCLUDE/PT_global_defs.h
ui_species.o: $(ARBHOME)/INCLUDE/smartptr.h
ui_species.o: $(ARBHOME)/INCLUDE/static_assert.h
ui_species.o: $(ARBHOME)/INCLUDE/test_global.h
