# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .cxx .c .depend

CPP_OBJECTS =   \
	GEN_interface.o \
	GEN_map.o \
	GEN_graphic.o \
	GEN_gene.o \
	GEN_nds.o \
	GEN_translations.o \
	EXP_interface.o \
	EXP_main.o \

$(MAIN): $(CPP_OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

.c.o:
	$(A_CC) $(cflags) -c $< $(CC_INCLUDES) $(POST_COMPILE) $<

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
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

EXP_interface.o: EXP.hxx
EXP_interface.o: EXP_local.hxx
EXP_interface.o: GEN.hxx
EXP_interface.o: GEN_local.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/ad_prot.h
EXP_interface.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
EXP_interface.o: $(ARBHOME)/INCLUDE/arb_assert.h
EXP_interface.o: $(ARBHOME)/INCLUDE/arb_core.h
EXP_interface.o: $(ARBHOME)/INCLUDE/arb_error.h
EXP_interface.o: $(ARBHOME)/INCLUDE/arb_msg.h
EXP_interface.o: $(ARBHOME)/INCLUDE/arb_string.h
EXP_interface.o: $(ARBHOME)/INCLUDE/arb_unordered_map.h
EXP_interface.o: $(ARBHOME)/INCLUDE/arbdb.h
EXP_interface.o: $(ARBHOME)/INCLUDE/arbdb_base.h
EXP_interface.o: $(ARBHOME)/INCLUDE/arbdbt.h
EXP_interface.o: $(ARBHOME)/INCLUDE/arbtools.h
EXP_interface.o: $(ARBHOME)/INCLUDE/attributes.h
EXP_interface.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/aw_base.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/aw_question.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/aw_root.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/aw_select.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/aw_window.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/cb.h
EXP_interface.o: $(ARBHOME)/INCLUDE/cb_base.h
EXP_interface.o: $(ARBHOME)/INCLUDE/cbtypes.h
EXP_interface.o: $(ARBHOME)/INCLUDE/cxxforward.h
EXP_interface.o: $(ARBHOME)/INCLUDE/db_query.h
EXP_interface.o: $(ARBHOME)/INCLUDE/db_scanner.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/dbui.h
EXP_interface.o: $(ARBHOME)/INCLUDE/downcast.h
EXP_interface.o: $(ARBHOME)/INCLUDE/dupstr.h
EXP_interface.o: $(ARBHOME)/INCLUDE/gccver.h
EXP_interface.o: $(ARBHOME)/INCLUDE/info_window.h
EXP_interface.o: $(ARBHOME)/INCLUDE/item_sel_list.h
EXP_interface.o: $(ARBHOME)/INCLUDE/items.h
EXP_interface.o: $(ARBHOME)/INCLUDE/smartptr.h
EXP_interface.o: $(ARBHOME)/INCLUDE/static_assert.h
EXP_interface.o: $(ARBHOME)/INCLUDE/test_global.h
EXP_interface.o: $(ARBHOME)/INCLUDE/ttypes.h

EXP_main.o: EXP.hxx
EXP_main.o: EXP_local.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
EXP_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
EXP_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
EXP_main.o: $(ARBHOME)/INCLUDE/arb_core.h
EXP_main.o: $(ARBHOME)/INCLUDE/arb_error.h
EXP_main.o: $(ARBHOME)/INCLUDE/arb_msg.h
EXP_main.o: $(ARBHOME)/INCLUDE/arb_string.h
EXP_main.o: $(ARBHOME)/INCLUDE/arbdb.h
EXP_main.o: $(ARBHOME)/INCLUDE/arbdb_base.h
EXP_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
EXP_main.o: $(ARBHOME)/INCLUDE/arbtools.h
EXP_main.o: $(ARBHOME)/INCLUDE/attributes.h
EXP_main.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/aw_base.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/awt.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/awt_input_mask.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/cb.h
EXP_main.o: $(ARBHOME)/INCLUDE/cb_base.h
EXP_main.o: $(ARBHOME)/INCLUDE/cbtypes.h
EXP_main.o: $(ARBHOME)/INCLUDE/cxxforward.h
EXP_main.o: $(ARBHOME)/INCLUDE/db_query.h
EXP_main.o: $(ARBHOME)/INCLUDE/downcast.h
EXP_main.o: $(ARBHOME)/INCLUDE/dupstr.h
EXP_main.o: $(ARBHOME)/INCLUDE/gccver.h
EXP_main.o: $(ARBHOME)/INCLUDE/items.h
EXP_main.o: $(ARBHOME)/INCLUDE/rootAsWin.h
EXP_main.o: $(ARBHOME)/INCLUDE/smartptr.h
EXP_main.o: $(ARBHOME)/INCLUDE/static_assert.h
EXP_main.o: $(ARBHOME)/INCLUDE/test_global.h
EXP_main.o: $(ARBHOME)/INCLUDE/ttypes.h

GEN_gene.o: GEN.hxx
GEN_gene.o: GEN_gene.hxx
GEN_gene.o: GEN_local.hxx
GEN_gene.o: GEN_nds.hxx
GEN_gene.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_gene.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GEN_gene.o: $(ARBHOME)/INCLUDE/adGene.h
GEN_gene.o: $(ARBHOME)/INCLUDE/arb_assert.h
GEN_gene.o: $(ARBHOME)/INCLUDE/arb_core.h
GEN_gene.o: $(ARBHOME)/INCLUDE/arb_error.h
GEN_gene.o: $(ARBHOME)/INCLUDE/arb_msg.h
GEN_gene.o: $(ARBHOME)/INCLUDE/arb_string.h
GEN_gene.o: $(ARBHOME)/INCLUDE/arbdb.h
GEN_gene.o: $(ARBHOME)/INCLUDE/arbdb_base.h
GEN_gene.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_gene.o: $(ARBHOME)/INCLUDE/arbtools.h
GEN_gene.o: $(ARBHOME)/INCLUDE/attributes.h
GEN_gene.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
GEN_gene.o: $(ARBHOME)/INCLUDE/aw_base.hxx
GEN_gene.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
GEN_gene.o: $(ARBHOME)/INCLUDE/aw_position.hxx
GEN_gene.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GEN_gene.o: $(ARBHOME)/INCLUDE/cb.h
GEN_gene.o: $(ARBHOME)/INCLUDE/cb_base.h
GEN_gene.o: $(ARBHOME)/INCLUDE/cbtypes.h
GEN_gene.o: $(ARBHOME)/INCLUDE/cxxforward.h
GEN_gene.o: $(ARBHOME)/INCLUDE/downcast.h
GEN_gene.o: $(ARBHOME)/INCLUDE/dupstr.h
GEN_gene.o: $(ARBHOME)/INCLUDE/gccver.h
GEN_gene.o: $(ARBHOME)/INCLUDE/items.h
GEN_gene.o: $(ARBHOME)/INCLUDE/smartptr.h
GEN_gene.o: $(ARBHOME)/INCLUDE/static_assert.h
GEN_gene.o: $(ARBHOME)/INCLUDE/test_global.h
GEN_gene.o: $(ARBHOME)/INCLUDE/ttypes.h

GEN_graphic.o: GEN.hxx
GEN_graphic.o: GEN_gene.hxx
GEN_graphic.o: GEN_graphic.hxx
GEN_graphic.o: GEN_local.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/arb_assert.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/arb_core.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/arb_error.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/arb_msg.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/arb_string.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/arbdb.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/arbdb_base.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/arbtools.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/attributes.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_base.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_position.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/cb.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/cb_base.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/cbtypes.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/cxxforward.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/downcast.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/dupstr.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/gccver.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/items.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/smartptr.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/static_assert.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/test_global.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/ttypes.h

GEN_interface.o: GEN.hxx
GEN_interface.o: GEN_local.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_interface.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GEN_interface.o: $(ARBHOME)/INCLUDE/adGene.h
GEN_interface.o: $(ARBHOME)/INCLUDE/arb_assert.h
GEN_interface.o: $(ARBHOME)/INCLUDE/arb_core.h
GEN_interface.o: $(ARBHOME)/INCLUDE/arb_error.h
GEN_interface.o: $(ARBHOME)/INCLUDE/arb_msg.h
GEN_interface.o: $(ARBHOME)/INCLUDE/arb_strarray.h
GEN_interface.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
GEN_interface.o: $(ARBHOME)/INCLUDE/arb_string.h
GEN_interface.o: $(ARBHOME)/INCLUDE/arb_unordered_map.h
GEN_interface.o: $(ARBHOME)/INCLUDE/arbdb.h
GEN_interface.o: $(ARBHOME)/INCLUDE/arbdb_base.h
GEN_interface.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_interface.o: $(ARBHOME)/INCLUDE/arbtools.h
GEN_interface.o: $(ARBHOME)/INCLUDE/attributes.h
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_base.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_question.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_select.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/cb.h
GEN_interface.o: $(ARBHOME)/INCLUDE/cb_base.h
GEN_interface.o: $(ARBHOME)/INCLUDE/cbtypes.h
GEN_interface.o: $(ARBHOME)/INCLUDE/cxxforward.h
GEN_interface.o: $(ARBHOME)/INCLUDE/db_query.h
GEN_interface.o: $(ARBHOME)/INCLUDE/db_scanner.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/dbui.h
GEN_interface.o: $(ARBHOME)/INCLUDE/downcast.h
GEN_interface.o: $(ARBHOME)/INCLUDE/dupstr.h
GEN_interface.o: $(ARBHOME)/INCLUDE/gccver.h
GEN_interface.o: $(ARBHOME)/INCLUDE/info_window.h
GEN_interface.o: $(ARBHOME)/INCLUDE/item_sel_list.h
GEN_interface.o: $(ARBHOME)/INCLUDE/items.h
GEN_interface.o: $(ARBHOME)/INCLUDE/Location.h
GEN_interface.o: $(ARBHOME)/INCLUDE/smartptr.h
GEN_interface.o: $(ARBHOME)/INCLUDE/static_assert.h
GEN_interface.o: $(ARBHOME)/INCLUDE/test_global.h
GEN_interface.o: $(ARBHOME)/INCLUDE/ttypes.h

GEN_map.o: EXP.hxx
GEN_map.o: EXP_local.hxx
GEN_map.o: GEN.hxx
GEN_map.o: GEN_gene.hxx
GEN_map.o: GEN_graphic.hxx
GEN_map.o: GEN_local.hxx
GEN_map.o: GEN_nds.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/ad_cb.h
GEN_map.o: $(ARBHOME)/INCLUDE/ad_cb_prot.h
GEN_map.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_map.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GEN_map.o: $(ARBHOME)/INCLUDE/adGene.h
GEN_map.o: $(ARBHOME)/INCLUDE/arb_assert.h
GEN_map.o: $(ARBHOME)/INCLUDE/arb_core.h
GEN_map.o: $(ARBHOME)/INCLUDE/arb_error.h
GEN_map.o: $(ARBHOME)/INCLUDE/arb_msg.h
GEN_map.o: $(ARBHOME)/INCLUDE/arb_progress.h
GEN_map.o: $(ARBHOME)/INCLUDE/arb_string.h
GEN_map.o: $(ARBHOME)/INCLUDE/arbdb.h
GEN_map.o: $(ARBHOME)/INCLUDE/arbdb_base.h
GEN_map.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_map.o: $(ARBHOME)/INCLUDE/arbtools.h
GEN_map.o: $(ARBHOME)/INCLUDE/attributes.h
GEN_map.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_base.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_position.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_question.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/awt.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/awt_input_mask.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/cb.h
GEN_map.o: $(ARBHOME)/INCLUDE/cb_base.h
GEN_map.o: $(ARBHOME)/INCLUDE/cbtypes.h
GEN_map.o: $(ARBHOME)/INCLUDE/cxxforward.h
GEN_map.o: $(ARBHOME)/INCLUDE/db_query.h
GEN_map.o: $(ARBHOME)/INCLUDE/dbui.h
GEN_map.o: $(ARBHOME)/INCLUDE/downcast.h
GEN_map.o: $(ARBHOME)/INCLUDE/dupstr.h
GEN_map.o: $(ARBHOME)/INCLUDE/gccver.h
GEN_map.o: $(ARBHOME)/INCLUDE/items.h
GEN_map.o: $(ARBHOME)/INCLUDE/mode_text.h
GEN_map.o: $(ARBHOME)/INCLUDE/rootAsWin.h
GEN_map.o: $(ARBHOME)/INCLUDE/smartptr.h
GEN_map.o: $(ARBHOME)/INCLUDE/static_assert.h
GEN_map.o: $(ARBHOME)/INCLUDE/test_global.h
GEN_map.o: $(ARBHOME)/INCLUDE/ttypes.h

GEN_nds.o: GEN.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/ad_cb.h
GEN_nds.o: $(ARBHOME)/INCLUDE/ad_cb_prot.h
GEN_nds.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_nds.o: $(ARBHOME)/INCLUDE/arb_assert.h
GEN_nds.o: $(ARBHOME)/INCLUDE/arb_core.h
GEN_nds.o: $(ARBHOME)/INCLUDE/arb_error.h
GEN_nds.o: $(ARBHOME)/INCLUDE/arb_msg.h
GEN_nds.o: $(ARBHOME)/INCLUDE/arb_string.h
GEN_nds.o: $(ARBHOME)/INCLUDE/arbdb.h
GEN_nds.o: $(ARBHOME)/INCLUDE/arbdb_base.h
GEN_nds.o: $(ARBHOME)/INCLUDE/arbtools.h
GEN_nds.o: $(ARBHOME)/INCLUDE/attributes.h
GEN_nds.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/aw_base.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/aw_select.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/cb.h
GEN_nds.o: $(ARBHOME)/INCLUDE/cb_base.h
GEN_nds.o: $(ARBHOME)/INCLUDE/cbtypes.h
GEN_nds.o: $(ARBHOME)/INCLUDE/cxxforward.h
GEN_nds.o: $(ARBHOME)/INCLUDE/dupstr.h
GEN_nds.o: $(ARBHOME)/INCLUDE/gccver.h
GEN_nds.o: $(ARBHOME)/INCLUDE/item_sel_list.h
GEN_nds.o: $(ARBHOME)/INCLUDE/items.h
GEN_nds.o: $(ARBHOME)/INCLUDE/nds.h
GEN_nds.o: $(ARBHOME)/INCLUDE/smartptr.h
GEN_nds.o: $(ARBHOME)/INCLUDE/static_assert.h
GEN_nds.o: $(ARBHOME)/INCLUDE/test_global.h
GEN_nds.o: $(ARBHOME)/INCLUDE/ttypes.h

GEN_translations.o: GEN.hxx
GEN_translations.o: GEN_local.hxx
GEN_translations.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_translations.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GEN_translations.o: $(ARBHOME)/INCLUDE/AP_codon_table.hxx
GEN_translations.o: $(ARBHOME)/INCLUDE/arb_assert.h
GEN_translations.o: $(ARBHOME)/INCLUDE/arb_core.h
GEN_translations.o: $(ARBHOME)/INCLUDE/arb_error.h
GEN_translations.o: $(ARBHOME)/INCLUDE/arb_msg.h
GEN_translations.o: $(ARBHOME)/INCLUDE/arb_string.h
GEN_translations.o: $(ARBHOME)/INCLUDE/arbdb.h
GEN_translations.o: $(ARBHOME)/INCLUDE/arbdb_base.h
GEN_translations.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_translations.o: $(ARBHOME)/INCLUDE/arbtools.h
GEN_translations.o: $(ARBHOME)/INCLUDE/attributes.h
GEN_translations.o: $(ARBHOME)/INCLUDE/aw_base.hxx
GEN_translations.o: $(ARBHOME)/INCLUDE/aw_question.hxx
GEN_translations.o: $(ARBHOME)/INCLUDE/cb.h
GEN_translations.o: $(ARBHOME)/INCLUDE/cb_base.h
GEN_translations.o: $(ARBHOME)/INCLUDE/cbtypes.h
GEN_translations.o: $(ARBHOME)/INCLUDE/cxxforward.h
GEN_translations.o: $(ARBHOME)/INCLUDE/downcast.h
GEN_translations.o: $(ARBHOME)/INCLUDE/dupstr.h
GEN_translations.o: $(ARBHOME)/INCLUDE/gccver.h
GEN_translations.o: $(ARBHOME)/INCLUDE/items.h
GEN_translations.o: $(ARBHOME)/INCLUDE/smartptr.h
GEN_translations.o: $(ARBHOME)/INCLUDE/static_assert.h
GEN_translations.o: $(ARBHOME)/INCLUDE/test_global.h
GEN_translations.o: $(ARBHOME)/INCLUDE/Translate.hxx
GEN_translations.o: $(ARBHOME)/INCLUDE/ttypes.h
