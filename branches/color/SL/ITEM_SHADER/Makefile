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

field_shader.o: field_shader.h
field_shader.o: item_shader.h
field_shader.o: $(ARBHOME)/INCLUDE/ad_cb_prot.h
field_shader.o: $(ARBHOME)/INCLUDE/ad_prot.h
field_shader.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
field_shader.o: $(ARBHOME)/INCLUDE/arb_assert.h
field_shader.o: $(ARBHOME)/INCLUDE/arb_core.h
field_shader.o: $(ARBHOME)/INCLUDE/arb_error.h
field_shader.o: $(ARBHOME)/INCLUDE/arb_global_defs.h
field_shader.o: $(ARBHOME)/INCLUDE/arb_msg.h
field_shader.o: $(ARBHOME)/INCLUDE/arb_str.h
field_shader.o: $(ARBHOME)/INCLUDE/arb_string.h
field_shader.o: $(ARBHOME)/INCLUDE/arbdb.h
field_shader.o: $(ARBHOME)/INCLUDE/arbdb_base.h
field_shader.o: $(ARBHOME)/INCLUDE/arbdbt.h
field_shader.o: $(ARBHOME)/INCLUDE/arbtools.h
field_shader.o: $(ARBHOME)/INCLUDE/attributes.h
field_shader.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
field_shader.o: $(ARBHOME)/INCLUDE/aw_base.hxx
field_shader.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
field_shader.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
field_shader.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
field_shader.o: $(ARBHOME)/INCLUDE/aw_root.hxx
field_shader.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
field_shader.o: $(ARBHOME)/INCLUDE/aw_select.hxx
field_shader.o: $(ARBHOME)/INCLUDE/aw_window.hxx
field_shader.o: $(ARBHOME)/INCLUDE/cb.h
field_shader.o: $(ARBHOME)/INCLUDE/cb_base.h
field_shader.o: $(ARBHOME)/INCLUDE/cbtypes.h
field_shader.o: $(ARBHOME)/INCLUDE/cxxforward.h
field_shader.o: $(ARBHOME)/INCLUDE/downcast.h
field_shader.o: $(ARBHOME)/INCLUDE/dupstr.h
field_shader.o: $(ARBHOME)/INCLUDE/gccver.h
field_shader.o: $(ARBHOME)/INCLUDE/item_sel_list.h
field_shader.o: $(ARBHOME)/INCLUDE/items.h
field_shader.o: $(ARBHOME)/INCLUDE/smartptr.h
field_shader.o: $(ARBHOME)/INCLUDE/static_assert.h
field_shader.o: $(ARBHOME)/INCLUDE/test_global.h
field_shader.o: $(ARBHOME)/INCLUDE/test_unit.h
field_shader.o: $(ARBHOME)/INCLUDE/ttypes.h

item_shader.o: field_shader.h
item_shader.o: item_shader.h
item_shader.o: $(ARBHOME)/INCLUDE/ad_prot.h
item_shader.o: $(ARBHOME)/INCLUDE/arb_assert.h
item_shader.o: $(ARBHOME)/INCLUDE/arb_core.h
item_shader.o: $(ARBHOME)/INCLUDE/arb_error.h
item_shader.o: $(ARBHOME)/INCLUDE/arb_msg.h
item_shader.o: $(ARBHOME)/INCLUDE/arb_string.h
item_shader.o: $(ARBHOME)/INCLUDE/arbdb.h
item_shader.o: $(ARBHOME)/INCLUDE/arbdb_base.h
item_shader.o: $(ARBHOME)/INCLUDE/arbtools.h
item_shader.o: $(ARBHOME)/INCLUDE/attributes.h
item_shader.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
item_shader.o: $(ARBHOME)/INCLUDE/aw_base.hxx
item_shader.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
item_shader.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
item_shader.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
item_shader.o: $(ARBHOME)/INCLUDE/aw_root.hxx
item_shader.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
item_shader.o: $(ARBHOME)/INCLUDE/aw_select.hxx
item_shader.o: $(ARBHOME)/INCLUDE/aw_window.hxx
item_shader.o: $(ARBHOME)/INCLUDE/cb.h
item_shader.o: $(ARBHOME)/INCLUDE/cb_base.h
item_shader.o: $(ARBHOME)/INCLUDE/cbtypes.h
item_shader.o: $(ARBHOME)/INCLUDE/cxxforward.h
item_shader.o: $(ARBHOME)/INCLUDE/dupstr.h
item_shader.o: $(ARBHOME)/INCLUDE/gccver.h
item_shader.o: $(ARBHOME)/INCLUDE/items.h
item_shader.o: $(ARBHOME)/INCLUDE/smartptr.h
item_shader.o: $(ARBHOME)/INCLUDE/static_assert.h
item_shader.o: $(ARBHOME)/INCLUDE/test_global.h
item_shader.o: $(ARBHOME)/INCLUDE/test_unit.h
item_shader.o: $(ARBHOME)/INCLUDE/ttypes.h

ValueTuple.o: item_shader.h
ValueTuple.o: $(ARBHOME)/INCLUDE/arb_assert.h
ValueTuple.o: $(ARBHOME)/INCLUDE/arb_core.h
ValueTuple.o: $(ARBHOME)/INCLUDE/arb_msg.h
ValueTuple.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ValueTuple.o: $(ARBHOME)/INCLUDE/arbtools.h
ValueTuple.o: $(ARBHOME)/INCLUDE/attributes.h
ValueTuple.o: $(ARBHOME)/INCLUDE/aw_base.hxx
ValueTuple.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
ValueTuple.o: $(ARBHOME)/INCLUDE/cb.h
ValueTuple.o: $(ARBHOME)/INCLUDE/cb_base.h
ValueTuple.o: $(ARBHOME)/INCLUDE/cbtypes.h
ValueTuple.o: $(ARBHOME)/INCLUDE/cxxforward.h
ValueTuple.o: $(ARBHOME)/INCLUDE/dupstr.h
ValueTuple.o: $(ARBHOME)/INCLUDE/gccver.h
ValueTuple.o: $(ARBHOME)/INCLUDE/items.h
ValueTuple.o: $(ARBHOME)/INCLUDE/smartptr.h
ValueTuple.o: $(ARBHOME)/INCLUDE/static_assert.h
ValueTuple.o: $(ARBHOME)/INCLUDE/test_global.h
ValueTuple.o: $(ARBHOME)/INCLUDE/test_unit.h
ValueTuple.o: $(ARBHOME)/INCLUDE/ttypes.h
