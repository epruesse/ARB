# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

OBJECTS =	\
		AWT_asciiprint.o \
		AWT_attributes.o \
		AWT_canio.o \
		AWT_canvas.o \
		AWT_changekey.o \
		AWT_config_manager.o \
		AWT_db_browser.o \
		AWT_file_selection.o \
		AWT_hotkeys.o \
		AWT_input_mask.o \
		AWT_item_sel_list.o \
		AWT_iupac.o \
		AWT_macro.o \
		AWT_map_key.o \
		AWT_nds.o \
		AWT_query_and_functions.o \
		AWT_sel_boxes.o \
		AWT_seq_colors.o \
		AWT_tables.o \
		AWT_www.o \

$(MAIN): $(OBJECTS)
	$(LINK_SHARED_LIB) $(@:.a=).$(SHARED_LIB_SUFFIX) $(OBJECTS)
	touch $@

.cxx.o:
	$(CPPLIB) $(cflags) -c $<  $(CPPINCLUDES)

#	$(CPPLIB) $(cflags) -c -D_USE_AW_WINDOW $<  $(CPPINCLUDES)

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

AWT_asciiprint.o: awt.hxx
AWT_asciiprint.o: awt_asciiprint.hxx
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_attributes.o: awt_attributes.hxx
AWT_attributes.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_attributes.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_attributes.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_attributes.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_attributes.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_attributes.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_attributes.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_attributes.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_attributes.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AWT_attributes.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_attributes.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_attributes.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_canio.o: awt.hxx
AWT_canio.o: awt_canvas.hxx
AWT_canio.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_canio.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_canio.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_canio.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_canio.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_canio.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_canio.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_canio.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_canio.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_canio.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_canio.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_canio.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_canio.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_canio.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_canio.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_canio.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_canio.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_canvas.o: awt.hxx
AWT_canvas.o: awt_canvas.hxx
AWT_canvas.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_canvas.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_canvas.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_canvas.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_canvas.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_canvas.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_changekey.o: awt.hxx
AWT_changekey.o: awt_item_sel_list.hxx
AWT_changekey.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_changekey.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_changekey.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_changekey.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_changekey.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_changekey.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_changekey.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_changekey.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_changekey.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_changekey.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_changekey.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_changekey.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_changekey.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_changekey.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_changekey.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_changekey.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_changekey.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_changekey.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_config_manager.o: awt.hxx
AWT_config_manager.o: awt_config_manager.hxx
AWT_config_manager.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_config_manager.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_config_manager.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_config_manager.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_config_manager.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_config_manager.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_config_manager.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_config_manager.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_config_manager.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_config_manager.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_config_manager.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_config_manager.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_config_manager.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_config_manager.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_config_manager.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_db_browser.o: awt.hxx
AWT_db_browser.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_db_browser.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_db_browser.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_db_browser.o: $(ARBHOME)/INCLUDE/AliView.hxx
AWT_db_browser.o: $(ARBHOME)/INCLUDE/AP_sequence.hxx
AWT_db_browser.o: $(ARBHOME)/INCLUDE/AP_Tree.hxx
AWT_db_browser.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_db_browser.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_db_browser.o: $(ARBHOME)/INCLUDE/ARB_Tree.hxx
AWT_db_browser.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_db_browser.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_db_browser.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_db_browser.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_db_browser.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_db_browser.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AWT_db_browser.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_db_browser.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_db_browser.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_db_browser.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_db_browser.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_db_browser.o: $(ARBHOME)/INCLUDE/downcast.h
AWT_db_browser.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_db_browser.o: $(ARBHOME)/INCLUDE/inline.h
AWT_db_browser.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_file_selection.o: awt.hxx
AWT_file_selection.o: awtlocal.hxx
AWT_file_selection.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_file_selection.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_file_selection.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_file_selection.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_file_selection.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_file_selection.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_file_selection.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_hotkeys.o: awt_hotkeys.hxx

AWT_input_mask.o: awt.hxx
AWT_input_mask.o: awt_hotkeys.hxx
AWT_input_mask.o: awt_input_mask.hxx
AWT_input_mask.o: awt_input_mask_internal.hxx
AWT_input_mask.o: awt_www.hxx
AWT_input_mask.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_input_mask.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_input_mask.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_input_mask.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_input_mask.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_input_mask.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_input_mask.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_input_mask.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_input_mask.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_input_mask.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_input_mask.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_input_mask.o: $(ARBHOME)/INCLUDE/aw_edit.hxx
AWT_input_mask.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_input_mask.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_input_mask.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_input_mask.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_input_mask.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_input_mask.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_item_sel_list.o: awt.hxx
AWT_item_sel_list.o: awt_item_sel_list.hxx
AWT_item_sel_list.o: awtlocal.hxx
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_item_sel_list.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_iupac.o: awt_iupac.hxx
AWT_iupac.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_iupac.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_iupac.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_iupac.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_iupac.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_iupac.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_iupac.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_iupac.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_iupac.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_iupac.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_macro.o: awt.hxx
AWT_macro.o: awt_macro.hxx
AWT_macro.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_macro.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_macro.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_macro.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_macro.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_macro.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_macro.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_macro.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_macro.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_macro.o: $(ARBHOME)/INCLUDE/aw_edit.hxx
AWT_macro.o: $(ARBHOME)/INCLUDE/aw_global.hxx
AWT_macro.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_macro.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_macro.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_macro.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_macro.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_macro.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_map_key.o: awt_map_key.hxx
AWT_map_key.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_map_key.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_map_key.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_map_key.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_map_key.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_map_key.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_map_key.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_map_key.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_map_key.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_map_key.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_map_key.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_map_key.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_map_key.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_nds.o: awt.hxx
AWT_nds.o: awt_config_manager.hxx
AWT_nds.o: awt_nds.hxx
AWT_nds.o: awt_sel_boxes.hxx
AWT_nds.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_nds.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_nds.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_nds.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_nds.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_nds.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_nds.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_nds.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_nds.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_nds.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_nds.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_nds.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_nds.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_nds.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_nds.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_nds.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_nds.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_nds.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_query_and_functions.o: awt.hxx
AWT_query_and_functions.o: awt_config_manager.hxx
AWT_query_and_functions.o: awt_item_sel_list.hxx
AWT_query_and_functions.o: awt_sel_boxes.hxx
AWT_query_and_functions.o: awtlocal.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/adGene.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_advice.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/GEN.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_sel_boxes.o: awt.hxx
AWT_sel_boxes.o: awt_item_sel_list.hxx
AWT_sel_boxes.o: awt_sel_boxes.hxx
AWT_sel_boxes.o: awtlocal.hxx
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/ad_config.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/aw_global.hxx
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_seq_colors.o: awt.hxx
AWT_seq_colors.o: awt_seq_colors.hxx
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_tables.o: awt.hxx
AWT_tables.o: awt_sel_boxes.hxx
AWT_tables.o: awtlocal.hxx
AWT_tables.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_tables.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_tables.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_tables.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_tables.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_tables.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_tables.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_tables.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_tables.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_tables.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_tables.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_tables.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_tables.o: $(ARBHOME)/INCLUDE/aw_global.hxx
AWT_tables.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_tables.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_tables.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_tables.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_tables.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_tables.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_www.o: awt.hxx
AWT_www.o: awt_config_manager.hxx
AWT_www.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_www.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_www.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_www.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_www.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_www.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_www.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_www.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_www.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_www.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_www.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_www.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_www.o: $(ARBHOME)/INCLUDE/aw_global_awars.hxx
AWT_www.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_www.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_www.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_www.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_www.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_www.o: $(ARBHOME)/INCLUDE/smartptr.h
