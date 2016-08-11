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

db_scanner.o: db_scanner.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/ad_cb.h
db_scanner.o: $(ARBHOME)/INCLUDE/ad_cb_prot.h
db_scanner.o: $(ARBHOME)/INCLUDE/ad_prot.h
db_scanner.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
db_scanner.o: $(ARBHOME)/INCLUDE/arb_assert.h
db_scanner.o: $(ARBHOME)/INCLUDE/arb_core.h
db_scanner.o: $(ARBHOME)/INCLUDE/arb_defs.h
db_scanner.o: $(ARBHOME)/INCLUDE/arb_error.h
db_scanner.o: $(ARBHOME)/INCLUDE/arb_mem.h
db_scanner.o: $(ARBHOME)/INCLUDE/arb_msg.h
db_scanner.o: $(ARBHOME)/INCLUDE/arb_progress.h
db_scanner.o: $(ARBHOME)/INCLUDE/arb_str.h
db_scanner.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
db_scanner.o: $(ARBHOME)/INCLUDE/arb_string.h
db_scanner.o: $(ARBHOME)/INCLUDE/arbdb.h
db_scanner.o: $(ARBHOME)/INCLUDE/arbdb_base.h
db_scanner.o: $(ARBHOME)/INCLUDE/arbdbt.h
db_scanner.o: $(ARBHOME)/INCLUDE/arbtools.h
db_scanner.o: $(ARBHOME)/INCLUDE/attributes.h
db_scanner.o: $(ARBHOME)/INCLUDE/aw_action.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_base.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_common.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_device.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_element.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_help.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_position.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_root.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_select.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_window.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/cb.h
db_scanner.o: $(ARBHOME)/INCLUDE/cb_base.h
db_scanner.o: $(ARBHOME)/INCLUDE/cbtypes.h
db_scanner.o: $(ARBHOME)/INCLUDE/cxxforward.h
db_scanner.o: $(ARBHOME)/INCLUDE/downcast.h
db_scanner.o: $(ARBHOME)/INCLUDE/dupstr.h
db_scanner.o: $(ARBHOME)/INCLUDE/gccver.h
db_scanner.o: $(ARBHOME)/INCLUDE/items.h
db_scanner.o: $(ARBHOME)/INCLUDE/smartptr.h
db_scanner.o: $(ARBHOME)/INCLUDE/static_assert.h
db_scanner.o: $(ARBHOME)/INCLUDE/test_global.h
db_scanner.o: $(ARBHOME)/INCLUDE/ttypes.h
