# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

OBJECTS =	\
		AWT_input_mask.o AWT_hotkeys.o \
		AWT_www.o AWT_tables.o AWT_irstree.o \
		AWT_asciiprint.o AWT_map_key.o AWT_tree_cmp.o AWT_seq_colors.o \
		AWT_pro_a_nucs.o AWT_csp.o AWT_canvas.o AWT_dtree.o AWT_tree.o AWT_tree_cb.o AWT_nei.o \
		AWT_filter.o AWT_sel_boxes.o AWT_canio.o AWT_nds.o \
		AWT_query_and_functions.o AWT_file_selection.o \
		AWT_imp_exp.o AWT_preset.o AWT_seq_dna.o AWT_seq_protein.o AWT_seq_simple_pro.o \
		AWT_codon_table.o AWT_iupac.o AWT_config_manager.o AWT_advice.o AWT_db_browser.o \
		AWT_attributes.o AWT_changekey.o AWT_item_sel_list.o AWT_macro.o AWT_translate.o

$(MAIN): $(OBJECTS)
	$(ARCPPLIB) $(@:.a=).$(SHARED_LIB_SUFFIX) $(OBJECTS)
	touch $@

.cxx.o:
	$(CCPLIB) $(cflags) -c $<  $(CPPINCLUDES)

#	$(CCPLIB) $(cflags) -c -D_USE_AW_WINDOW $<  $(CPPINCLUDES)

awt_tree_cb.hxx: AWT_tree_cb.cxx
	rm -f $@
	../MAKEBIN/aisc_mkpt -C $? >$@

clean:
	rm -f $(OBJECTS) *.a *.so

DEPENDS = $(OBJECTS:.o=.depend)
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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

AWT_advice.o: awt.hxx
AWT_advice.o: awt_advice.hxx
AWT_advice.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_advice.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_advice.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_advice.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_advice.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_advice.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_advice.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_advice.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_asciiprint.o: awt.hxx
AWT_asciiprint.o: awt_asciiprint.hxx
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_attributes.o: awt_attributes.hxx
AWT_attributes.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_attributes.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_attributes.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_attributes.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AWT_attributes.o: $(ARBHOME)/INCLUDE/aw_root.hxx

AWT_canio.o: awt.hxx
AWT_canio.o: awt_canvas.hxx
AWT_canio.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_canio.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_canio.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_canio.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_canio.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_canio.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_canio.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_canio.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_canio.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_canio.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_canvas.o: awt.hxx
AWT_canvas.o: awt_canvas.hxx
AWT_canvas.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_canvas.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_canvas.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_canvas.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_canvas.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_changekey.o: awt.hxx
AWT_changekey.o: awt_changekey.hxx
AWT_changekey.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_changekey.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_changekey.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_changekey.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_changekey.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_changekey.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_changekey.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_changekey.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_changekey.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_changekey.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_changekey.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_codon_table.o: awt.hxx
AWT_codon_table.o: awt_codon_table.hxx
AWT_codon_table.o: awt_iupac.hxx
AWT_codon_table.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_codon_table.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_codon_table.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_codon_table.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_codon_table.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_codon_table.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_codon_table.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_codon_table.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_config_manager.o: awt.hxx
AWT_config_manager.o: awt_config_manager.hxx
AWT_config_manager.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_config_manager.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_config_manager.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_config_manager.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_config_manager.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_config_manager.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_config_manager.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_config_manager.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_csp.o: awt.hxx
AWT_csp.o: awt_csp.hxx
AWT_csp.o: awt_pro_a_nucs.hxx
AWT_csp.o: awt_sel_boxes.hxx
AWT_csp.o: awt_tree.hxx
AWT_csp.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_csp.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_csp.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_csp.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_csp.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_csp.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AWT_csp.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_csp.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_csp.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_csp.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_csp.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_csp.o: $(ARBHOME)/INCLUDE/BI_helix.hxx

AWT_db_browser.o: awt.hxx
AWT_db_browser.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_db_browser.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_db_browser.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_db_browser.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_db_browser.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_db_browser.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_db_browser.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_db_browser.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_db_browser.o: $(ARBHOME)/INCLUDE/inline.h

AWT_dtree.o: awt.hxx
AWT_dtree.o: awt_attributes.hxx
AWT_dtree.o: awt_canvas.hxx
AWT_dtree.o: awt_dtree.hxx
AWT_dtree.o: awt_nds.hxx
AWT_dtree.o: awt_pro_a_nucs.hxx
AWT_dtree.o: awt_tree.hxx
AWT_dtree.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_dtree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_dtree.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_dtree.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_dtree.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_dtree.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_dtree.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AWT_dtree.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_dtree.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_dtree.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_dtree.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
AWT_dtree.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_dtree.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_file_selection.o: awt.hxx
AWT_file_selection.o: awtlocal.hxx
AWT_file_selection.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_file_selection.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_file_selection.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_file_selection.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_file_selection.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_file_selection.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_filter.o: awt.hxx
AWT_filter.o: awt_pro_a_nucs.hxx
AWT_filter.o: awt_sel_boxes.hxx
AWT_filter.o: awt_tree.hxx
AWT_filter.o: awtfilter.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_filter.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_hotkeys.o: awt_hotkeys.hxx

AWT_imp_exp.o: awt.hxx
AWT_imp_exp.o: awt_nds.hxx
AWT_imp_exp.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_imp_exp.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_imp_exp.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_imp_exp.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_imp_exp.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_imp_exp.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_imp_exp.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_imp_exp.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_imp_exp.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_imp_exp.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_imp_exp.o: $(ARBHOME)/INCLUDE/xml.hxx

AWT_input_mask.o: awt.hxx
AWT_input_mask.o: awt_changekey.hxx
AWT_input_mask.o: awt_hotkeys.hxx
AWT_input_mask.o: awt_input_mask.hxx
AWT_input_mask.o: awt_www.hxx
AWT_input_mask.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_input_mask.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_input_mask.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_input_mask.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_input_mask.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_input_mask.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_input_mask.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_input_mask.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_input_mask.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_irstree.o: awt_canvas.hxx
AWT_irstree.o: awt_dtree.hxx
AWT_irstree.o: awt_irstree.hxx
AWT_irstree.o: awt_nds.hxx
AWT_irstree.o: awt_pro_a_nucs.hxx
AWT_irstree.o: awt_tree.hxx
AWT_irstree.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_irstree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_irstree.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_irstree.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_irstree.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_irstree.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_irstree.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AWT_irstree.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_irstree.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_irstree.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_irstree.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_irstree.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_item_sel_list.o: awt.hxx
AWT_item_sel_list.o: awt_item_sel_list.hxx
AWT_item_sel_list.o: awtlocal.hxx
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_iupac.o: awt_iupac.hxx
AWT_iupac.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_iupac.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_iupac.o: $(ARBHOME)/INCLUDE/arbdb.h

AWT_macro.o: awt.hxx
AWT_macro.o: awt_macro.hxx
AWT_macro.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_macro.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_macro.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_macro.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_macro.o: $(ARBHOME)/INCLUDE/aw_global.hxx
AWT_macro.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_macro.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_macro.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_macro.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_map_key.o: awt_map_key.hxx
AWT_map_key.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_map_key.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_map_key.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_map_key.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_map_key.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_map_key.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_nds.o: awt.hxx
AWT_nds.o: awt_changekey.hxx
AWT_nds.o: awt_config_manager.hxx
AWT_nds.o: awt_nds.hxx
AWT_nds.o: awt_sel_boxes.hxx
AWT_nds.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_nds.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_nds.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_nds.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_nds.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_nds.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_nds.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_nds.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_nds.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_nds.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_nds.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_nei.o: awt_nei.hxx
AWT_nei.o: awt_pro_a_nucs.hxx
AWT_nei.o: awt_tree.hxx
AWT_nei.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_nei.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_nei.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_nei.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_nei.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_nei.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AWT_nei.o: $(ARBHOME)/INCLUDE/aw_root.hxx

AWT_preset.o: aw_def.hxx
AWT_preset.o: awt.hxx
AWT_preset.o: awt_advice.hxx
AWT_preset.o: awt_canvas.hxx
AWT_preset.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_preset.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_preset.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_preset.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AWT_preset.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_preset.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_preset.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_preset.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
AWT_preset.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_preset.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_pro_a_nucs.o: awt.hxx
AWT_pro_a_nucs.o: awt_codon_table.hxx
AWT_pro_a_nucs.o: awt_pro_a_nucs.hxx
AWT_pro_a_nucs.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_pro_a_nucs.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_pro_a_nucs.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_pro_a_nucs.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_pro_a_nucs.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_pro_a_nucs.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_pro_a_nucs.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_pro_a_nucs.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_pro_a_nucs.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_pro_a_nucs.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_query_and_functions.o: awt.hxx
AWT_query_and_functions.o: awt_changekey.hxx
AWT_query_and_functions.o: awt_config_manager.hxx
AWT_query_and_functions.o: awt_sel_boxes.hxx
AWT_query_and_functions.o: awtlocal.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/adGene.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/GEN.hxx

AWT_sel_boxes.o: awt.hxx
AWT_sel_boxes.o: awt_changekey.hxx
AWT_sel_boxes.o: awt_sel_boxes.hxx
AWT_sel_boxes.o: awtlocal.hxx
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/ad_config.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/aw_global.hxx
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_seq_colors.o: awt.hxx
AWT_seq_colors.o: awt_seq_colors.hxx
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_seq_dna.o: awt_parsimony_defaults.hxx
AWT_seq_dna.o: awt_pro_a_nucs.hxx
AWT_seq_dna.o: awt_seq_dna.hxx
AWT_seq_dna.o: awt_tree.hxx
AWT_seq_dna.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_seq_dna.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_seq_dna.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_seq_dna.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_seq_dna.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_seq_dna.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AWT_seq_dna.o: $(ARBHOME)/INCLUDE/aw_root.hxx

AWT_seq_protein.o: awt_parsimony_defaults.hxx
AWT_seq_protein.o: awt_pro_a_nucs.hxx
AWT_seq_protein.o: awt_seq_protein.hxx
AWT_seq_protein.o: awt_tree.hxx
AWT_seq_protein.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_seq_protein.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_seq_protein.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_seq_protein.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_seq_protein.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_seq_protein.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AWT_seq_protein.o: $(ARBHOME)/INCLUDE/aw_root.hxx

AWT_seq_simple_pro.o: awt_pro_a_nucs.hxx
AWT_seq_simple_pro.o: awt_seq_simple_pro.hxx
AWT_seq_simple_pro.o: awt_tree.hxx
AWT_seq_simple_pro.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_seq_simple_pro.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_seq_simple_pro.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_seq_simple_pro.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_seq_simple_pro.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_seq_simple_pro.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AWT_seq_simple_pro.o: $(ARBHOME)/INCLUDE/aw_root.hxx

AWT_tables.o: awt.hxx
AWT_tables.o: awt_sel_boxes.hxx
AWT_tables.o: awtlocal.hxx
AWT_tables.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_tables.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_tables.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_tables.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_tables.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_tables.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_tables.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_tables.o: $(ARBHOME)/INCLUDE/aw_global.hxx
AWT_tables.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_tables.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_tables.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_tables.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_translate.o: awt.hxx
AWT_translate.o: awt_codon_table.hxx
AWT_translate.o: awt_pro_a_nucs.hxx
AWT_translate.o: awt_translate.hxx
AWT_translate.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_translate.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_translate.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_translate.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_translate.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_translate.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_translate.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_translate.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_translate.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_translate.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_tree.o: awt.hxx
AWT_tree.o: awt_attributes.hxx
AWT_tree.o: awt_canvas.hxx
AWT_tree.o: awt_pro_a_nucs.hxx
AWT_tree.o: awt_tree.hxx
AWT_tree.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_tree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_tree.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_tree.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_tree.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_tree.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AWT_tree.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_tree.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_tree.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_tree.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_tree.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_tree_cb.o: awt.hxx
AWT_tree_cb.o: awt_advice.hxx
AWT_tree_cb.o: awt_canvas.hxx
AWT_tree_cb.o: awt_dtree.hxx
AWT_tree_cb.o: awt_pro_a_nucs.hxx
AWT_tree_cb.o: awt_tree.hxx
AWT_tree_cb.o: awt_tree_cb.hxx
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_tree_cmp.o: awt.hxx
AWT_tree_cmp.o: awt_pro_a_nucs.hxx
AWT_tree_cmp.o: awt_tree.hxx
AWT_tree_cmp.o: awt_tree_cmp.hxx
AWT_tree_cmp.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_tree_cmp.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_tree_cmp.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_tree_cmp.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_tree_cmp.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_tree_cmp.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AWT_tree_cmp.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_tree_cmp.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_tree_cmp.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_tree_cmp.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_tree_cmp.o: $(ARBHOME)/INCLUDE/aw_window.hxx

AWT_www.o: awt.hxx
AWT_www.o: awt_config_manager.hxx
AWT_www.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_www.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_www.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_www.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_www.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_www.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_www.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_www.o: $(ARBHOME)/INCLUDE/aw_global_awars.hxx
AWT_www.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_www.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_www.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_www.o: $(ARBHOME)/INCLUDE/aw_window.hxx
