# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .cxx .depend

CPP_OBJECTS =   \
	PRD_Pair.o \
	PRD_Item.o \
	PRD_Node.o \
	PRD_Range.o \
	PRD_SequenceIterator.o \
	PRD_SearchFIFO.o \
	PRD_Design.o \
	primer_design.o \

$(MAIN): $(CPP_OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)

./prd_test: $(MAIN) PRD_main.o
	$(LINK_EXECUTABLE) $@ PRD_main.o $(MAIN) $(EXECLIBS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

clean:
	rm -f $(CPP_OBJECTS) *.a

#.IGNORE:
#	clean

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

PRD_Design.o: PRD_Design.hxx
PRD_Design.o: PRD_Globals.hxx
PRD_Design.o: PRD_Item.hxx
PRD_Design.o: PRD_Node.hxx
PRD_Design.o: PRD_Pair.hxx
PRD_Design.o: PRD_Range.hxx
PRD_Design.o: PRD_SearchFIFO.hxx
PRD_Design.o: PRD_SequenceIterator.hxx
PRD_Design.o: $(ARBHOME)/INCLUDE/ad_prot.h
PRD_Design.o: $(ARBHOME)/INCLUDE/arb_assert.h
PRD_Design.o: $(ARBHOME)/INCLUDE/arb_core.h
PRD_Design.o: $(ARBHOME)/INCLUDE/arb_error.h
PRD_Design.o: $(ARBHOME)/INCLUDE/arb_msg.h
PRD_Design.o: $(ARBHOME)/INCLUDE/arb_progress.h
PRD_Design.o: $(ARBHOME)/INCLUDE/arb_string.h
PRD_Design.o: $(ARBHOME)/INCLUDE/arbdb.h
PRD_Design.o: $(ARBHOME)/INCLUDE/arbdb_base.h
PRD_Design.o: $(ARBHOME)/INCLUDE/arbtools.h
PRD_Design.o: $(ARBHOME)/INCLUDE/attributes.h
PRD_Design.o: $(ARBHOME)/INCLUDE/cxxforward.h
PRD_Design.o: $(ARBHOME)/INCLUDE/dupstr.h
PRD_Design.o: $(ARBHOME)/INCLUDE/gccver.h
PRD_Design.o: $(ARBHOME)/INCLUDE/smartptr.h
PRD_Design.o: $(ARBHOME)/INCLUDE/test_global.h

PRD_Item.o: PRD_Globals.hxx
PRD_Item.o: PRD_Item.hxx
PRD_Item.o: PRD_SequenceIterator.hxx
PRD_Item.o: $(ARBHOME)/INCLUDE/arb_assert.h
PRD_Item.o: $(ARBHOME)/INCLUDE/arb_core.h
PRD_Item.o: $(ARBHOME)/INCLUDE/arbtools.h
PRD_Item.o: $(ARBHOME)/INCLUDE/attributes.h
PRD_Item.o: $(ARBHOME)/INCLUDE/cxxforward.h
PRD_Item.o: $(ARBHOME)/INCLUDE/dupstr.h
PRD_Item.o: $(ARBHOME)/INCLUDE/gccver.h
PRD_Item.o: $(ARBHOME)/INCLUDE/test_global.h

PRD_Node.o: PRD_Globals.hxx
PRD_Node.o: PRD_Node.hxx
PRD_Node.o: $(ARBHOME)/INCLUDE/arb_assert.h
PRD_Node.o: $(ARBHOME)/INCLUDE/arb_core.h
PRD_Node.o: $(ARBHOME)/INCLUDE/arbtools.h
PRD_Node.o: $(ARBHOME)/INCLUDE/attributes.h
PRD_Node.o: $(ARBHOME)/INCLUDE/cxxforward.h
PRD_Node.o: $(ARBHOME)/INCLUDE/dupstr.h
PRD_Node.o: $(ARBHOME)/INCLUDE/gccver.h
PRD_Node.o: $(ARBHOME)/INCLUDE/test_global.h

PRD_Pair.o: PRD_Globals.hxx
PRD_Pair.o: PRD_Item.hxx
PRD_Pair.o: PRD_Pair.hxx
PRD_Pair.o: $(ARBHOME)/INCLUDE/arb_assert.h
PRD_Pair.o: $(ARBHOME)/INCLUDE/arb_core.h
PRD_Pair.o: $(ARBHOME)/INCLUDE/arbtools.h
PRD_Pair.o: $(ARBHOME)/INCLUDE/attributes.h
PRD_Pair.o: $(ARBHOME)/INCLUDE/cxxforward.h
PRD_Pair.o: $(ARBHOME)/INCLUDE/dupstr.h
PRD_Pair.o: $(ARBHOME)/INCLUDE/gccver.h
PRD_Pair.o: $(ARBHOME)/INCLUDE/test_global.h

PRD_Range.o: PRD_Globals.hxx
PRD_Range.o: PRD_Range.hxx
PRD_Range.o: $(ARBHOME)/INCLUDE/arb_assert.h
PRD_Range.o: $(ARBHOME)/INCLUDE/arb_core.h
PRD_Range.o: $(ARBHOME)/INCLUDE/arbtools.h
PRD_Range.o: $(ARBHOME)/INCLUDE/attributes.h
PRD_Range.o: $(ARBHOME)/INCLUDE/cxxforward.h
PRD_Range.o: $(ARBHOME)/INCLUDE/dupstr.h
PRD_Range.o: $(ARBHOME)/INCLUDE/gccver.h
PRD_Range.o: $(ARBHOME)/INCLUDE/test_global.h

PRD_SearchFIFO.o: PRD_Globals.hxx
PRD_SearchFIFO.o: PRD_Node.hxx
PRD_SearchFIFO.o: PRD_Range.hxx
PRD_SearchFIFO.o: PRD_SearchFIFO.hxx
PRD_SearchFIFO.o: $(ARBHOME)/INCLUDE/arb_assert.h
PRD_SearchFIFO.o: $(ARBHOME)/INCLUDE/arb_core.h
PRD_SearchFIFO.o: $(ARBHOME)/INCLUDE/arbtools.h
PRD_SearchFIFO.o: $(ARBHOME)/INCLUDE/attributes.h
PRD_SearchFIFO.o: $(ARBHOME)/INCLUDE/cxxforward.h
PRD_SearchFIFO.o: $(ARBHOME)/INCLUDE/dupstr.h
PRD_SearchFIFO.o: $(ARBHOME)/INCLUDE/gccver.h
PRD_SearchFIFO.o: $(ARBHOME)/INCLUDE/test_global.h

PRD_SequenceIterator.o: PRD_Globals.hxx
PRD_SequenceIterator.o: PRD_SequenceIterator.hxx
PRD_SequenceIterator.o: $(ARBHOME)/INCLUDE/arb_assert.h
PRD_SequenceIterator.o: $(ARBHOME)/INCLUDE/arb_core.h
PRD_SequenceIterator.o: $(ARBHOME)/INCLUDE/arbtools.h
PRD_SequenceIterator.o: $(ARBHOME)/INCLUDE/attributes.h
PRD_SequenceIterator.o: $(ARBHOME)/INCLUDE/cxxforward.h
PRD_SequenceIterator.o: $(ARBHOME)/INCLUDE/dupstr.h
PRD_SequenceIterator.o: $(ARBHOME)/INCLUDE/gccver.h
PRD_SequenceIterator.o: $(ARBHOME)/INCLUDE/test_global.h

primer_design.o: PRD_Design.hxx
primer_design.o: PRD_Globals.hxx
primer_design.o: PRD_Item.hxx
primer_design.o: PRD_Node.hxx
primer_design.o: PRD_Pair.hxx
primer_design.o: PRD_Range.hxx
primer_design.o: PRD_SequenceIterator.hxx
primer_design.o: primer_design.hxx
primer_design.o: $(ARBHOME)/INCLUDE/ad_prot.h
primer_design.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
primer_design.o: $(ARBHOME)/INCLUDE/adGene.h
primer_design.o: $(ARBHOME)/INCLUDE/arb_assert.h
primer_design.o: $(ARBHOME)/INCLUDE/arb_core.h
primer_design.o: $(ARBHOME)/INCLUDE/arb_defs.h
primer_design.o: $(ARBHOME)/INCLUDE/arb_error.h
primer_design.o: $(ARBHOME)/INCLUDE/arb_msg.h
primer_design.o: $(ARBHOME)/INCLUDE/arb_progress.h
primer_design.o: $(ARBHOME)/INCLUDE/arb_string.h
primer_design.o: $(ARBHOME)/INCLUDE/arbdb.h
primer_design.o: $(ARBHOME)/INCLUDE/arbdb_base.h
primer_design.o: $(ARBHOME)/INCLUDE/arbdbt.h
primer_design.o: $(ARBHOME)/INCLUDE/arbtools.h
primer_design.o: $(ARBHOME)/INCLUDE/attributes.h
primer_design.o: $(ARBHOME)/INCLUDE/aw_action.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_area_management.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_assert.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_base.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_cb_struct.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_clipable.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_common.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_common_impl.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_device.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_device_cairo.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_device_gtk.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_device_impl.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_device_print.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_device_size.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_element.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_font_limits.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_gtk_forward_declarations.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_gtk_migration_helpers.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_help.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_position.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_question.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_rgb.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_root.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_select.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_signal.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_simple_device.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_stylable.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_window.hxx
primer_design.o: $(ARBHOME)/INCLUDE/aw_zoomable.hxx
primer_design.o: $(ARBHOME)/INCLUDE/awt_config_manager.hxx
primer_design.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
primer_design.o: $(ARBHOME)/INCLUDE/cb.h
primer_design.o: $(ARBHOME)/INCLUDE/cb_base.h
primer_design.o: $(ARBHOME)/INCLUDE/cbtypes.h
primer_design.o: $(ARBHOME)/INCLUDE/cxxforward.h
primer_design.o: $(ARBHOME)/INCLUDE/downcast.h
primer_design.o: $(ARBHOME)/INCLUDE/dupstr.h
primer_design.o: $(ARBHOME)/INCLUDE/gccver.h
primer_design.o: $(ARBHOME)/INCLUDE/GEN.hxx
primer_design.o: $(ARBHOME)/INCLUDE/items.h
primer_design.o: $(ARBHOME)/INCLUDE/smartptr.h
primer_design.o: $(ARBHOME)/INCLUDE/static_assert.h
primer_design.o: $(ARBHOME)/INCLUDE/test_global.h
primer_design.o: $(ARBHOME)/INCLUDE/ttypes.h
