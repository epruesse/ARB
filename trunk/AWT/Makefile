# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx

OBJECTS =	\
		AWT_www.o AWT_tables.o AWT_irstree.o \
		AWT_asciiprint.o AWT_map_key.o AWT_tree_cmp.o AWT_seq_colors.o \
		AWT_pro_a_nucs.o AWT_csp.o AWT_canvas.o AWT_dtree.o AWT_tree.o AWT_tree_cb.o AWT_nei.o \
		BI_helix.o AWT_filter.o AWT_sel_boxes.o AWT_canio.o AWT_nds.o \
		AWT_query_and_functions.o AWT_file_selection.o AWT_species_sel_box.o \
		AWT_imp_exp.o AWT_preset.o AWT_seq_dna.o AWT_seq_protein.o AWT_seq_simple_pro.o \
		AWT_codon_table.o AWT_iupac.o

$(MAIN): $(OBJECTS)
	$(ARCPPLIB) $(@:.a=).$(SHARED_LIB_SUFFIX) $(OBJECTS)  
	touch $@

.cxx.o: 
	$(CCPLIB) $(cflags) -c -D_USE_AW_WINDOW $<  $(CPPINCLUDES)

awt_tree_cb.hxx: AWT_tree_cb.cxx
	rm -f $@
	../MAKEBIN/aisc_mkpt -C $? >$@

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) -D_USE_AW_WINDOW $(OBJECTS:.o=.cxx)

# DO NOT DELETE THIS LINE -- make depend depends on it.

AWT_www.o:  
AWT_www.o:  
AWT_www.o:  
AWT_www.o:  
AWT_www.o:   
AWT_www.o:  
AWT_www.o:  
AWT_www.o:  
AWT_www.o:   
AWT_www.o:  $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_www.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_www.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_www.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_www.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_www.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_www.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_www.o: $(ARBHOME)/INCLUDE/ad_t_prot.h ./awt.hxx
AWT_tables.o:  
AWT_tables.o:  
AWT_tables.o:  
AWT_tables.o:  
AWT_tables.o:  
AWT_tables.o:  
AWT_tables.o:  
AWT_tables.o:  
AWT_tables.o:  
AWT_tables.o:  
AWT_tables.o:  
AWT_tables.o:  
AWT_tables.o:  $(ARBHOME)/INCLUDE/arbdb.h
AWT_tables.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_tables.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_tables.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_tables.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_tables.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_tables.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_tables.o: $(ARBHOME)/INCLUDE/aw_awars.hxx ./awt.hxx
AWT_tables.o: awtlocal.hxx
AWT_irstree.o:  
AWT_irstree.o:  
AWT_irstree.o:  
AWT_irstree.o:  
AWT_irstree.o:  
AWT_irstree.o:  
AWT_irstree.o:  $(ARBHOME)/INCLUDE/arbdb.h
AWT_irstree.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_irstree.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_irstree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_irstree.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_irstree.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_irstree.o: $(ARBHOME)/INCLUDE/aw_window.hxx ./awt_canvas.hxx
AWT_irstree.o: ./awt_nds.hxx awt_tree.hxx ./awt_pro_a_nucs.hxx awt_dtree.hxx
AWT_irstree.o: awt_irstree.hxx $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_asciiprint.o:  
AWT_asciiprint.o:  
AWT_asciiprint.o:  
AWT_asciiprint.o:  
AWT_asciiprint.o:  
AWT_asciiprint.o:  
AWT_asciiprint.o:  
AWT_asciiprint.o:  
AWT_asciiprint.o:  
AWT_asciiprint.o: 
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_asciiprint.o: $(ARBHOME)/INCLUDE/aw_window.hxx ./awt.hxx
AWT_asciiprint.o: awt_asciiprint.hxx
AWT_map_key.o:  
AWT_map_key.o:  
AWT_map_key.o:  
AWT_map_key.o:  
AWT_map_key.o: 
AWT_map_key.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_map_key.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_map_key.o: $(ARBHOME)/INCLUDE/aw_window.hxx awt_map_key.hxx
AWT_tree_cmp.o:  
AWT_tree_cmp.o:  
AWT_tree_cmp.o:  
AWT_tree_cmp.o:  
AWT_tree_cmp.o:  
AWT_tree_cmp.o:  
AWT_tree_cmp.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_tree_cmp.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_tree_cmp.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_tree_cmp.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_tree_cmp.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_tree_cmp.o: $(ARBHOME)/INCLUDE/aw_window.hxx awt_tree.hxx
AWT_tree_cmp.o: ./awt_pro_a_nucs.hxx ./awt_tree_cmp.hxx
AWT_seq_colors.o:  
AWT_seq_colors.o:  
AWT_seq_colors.o:  
AWT_seq_colors.o:  
AWT_seq_colors.o:  
AWT_seq_colors.o:  
AWT_seq_colors.o:  
AWT_seq_colors.o:  
AWT_seq_colors.o:  
AWT_seq_colors.o:  
AWT_seq_colors.o: 
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_seq_colors.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_seq_colors.o: awt_seq_colors.hxx ./awt.hxx
AWT_pro_a_nucs.o:  
AWT_pro_a_nucs.o:  
AWT_pro_a_nucs.o:  
AWT_pro_a_nucs.o:  
AWT_pro_a_nucs.o:  
AWT_pro_a_nucs.o:  
AWT_pro_a_nucs.o:  
AWT_pro_a_nucs.o:  
AWT_pro_a_nucs.o:  
AWT_pro_a_nucs.o:  
AWT_pro_a_nucs.o: 
AWT_pro_a_nucs.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_pro_a_nucs.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_pro_a_nucs.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_pro_a_nucs.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_pro_a_nucs.o: ./awt_pro_a_nucs.hxx ./awt_codon_table.hxx ./awt.hxx
AWT_csp.o:  
AWT_csp.o:  
AWT_csp.o:  
AWT_csp.o:  
AWT_csp.o:   
AWT_csp.o:  
AWT_csp.o:  
AWT_csp.o:  
AWT_csp.o:   
AWT_csp.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_csp.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_csp.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_csp.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_csp.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_csp.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_csp.o: $(ARBHOME)/INCLUDE/aw_window.hxx ./awt.hxx
AWT_csp.o: awt_tree.hxx ./awt_pro_a_nucs.hxx BI_helix.hxx awt_csp.hxx
AWT_canvas.o:  
AWT_canvas.o:  
AWT_canvas.o:  
AWT_canvas.o:  
AWT_canvas.o:  $(ARBHOME)/INCLUDE/arbdb.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_canvas.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_canvas.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_canvas.o: $(ARBHOME)/INCLUDE/aw_window.hxx ./awt_canvas.hxx
AWT_dtree.o:  
AWT_dtree.o:  
AWT_dtree.o:  
AWT_dtree.o:  
AWT_dtree.o:  
AWT_dtree.o:  
AWT_dtree.o:  
AWT_dtree.o:  
AWT_dtree.o:  
AWT_dtree.o:  
AWT_dtree.o:  
AWT_dtree.o:  $(ARBHOME)/INCLUDE/arbdb.h
AWT_dtree.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_dtree.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_dtree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_dtree.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_dtree.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_dtree.o: $(ARBHOME)/INCLUDE/aw_window.hxx ./awt.hxx
AWT_dtree.o: ./awt_canvas.hxx ./awt_nds.hxx awt_tree.hxx ./awt_pro_a_nucs.hxx
AWT_dtree.o: awt_dtree.hxx $(ARBHOME)/INCLUDE/aw_preset.hxx
AWT_dtree.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_tree.o:  
AWT_tree.o:  
AWT_tree.o:  
AWT_tree.o:  
AWT_tree.o:  
AWT_tree.o:  
AWT_tree.o:  
AWT_tree.o:  
AWT_tree.o:  
AWT_tree.o:  
AWT_tree.o:  
AWT_tree.o:  
AWT_tree.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_tree.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_tree.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_tree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h 
AWT_tree.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_tree.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_tree.o: $(ARBHOME)/INCLUDE/aw_window.hxx ./awt_canvas.hxx
AWT_tree.o: awt_tree.hxx ./awt_pro_a_nucs.hxx
AWT_tree_cb.o:  
AWT_tree_cb.o:  
AWT_tree_cb.o:  
AWT_tree_cb.o:  
AWT_tree_cb.o:  
AWT_tree_cb.o:  
AWT_tree_cb.o:  
AWT_tree_cb.o:  
AWT_tree_cb.o:  
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_tree_cb.o: $(ARBHOME)/INCLUDE/aw_awars.hxx ./awt_canvas.hxx
AWT_tree_cb.o: awt_tree.hxx ./awt_pro_a_nucs.hxx awt_dtree.hxx
AWT_tree_cb.o: ./awt_tree_cb.hxx
AWT_nei.o:  
AWT_nei.o:  
AWT_nei.o:  
AWT_nei.o:  
AWT_nei.o:   
AWT_nei.o:  
AWT_nei.o:  
AWT_nei.o:  
AWT_nei.o:   
AWT_nei.o:  $(ARBHOME)/INCLUDE/arbdb.h
AWT_nei.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_nei.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_nei.o: $(ARBHOME)/INCLUDE/ad_t_prot.h awt_tree.hxx
AWT_nei.o: ./awt_pro_a_nucs.hxx awt_nei.hxx
BI_helix.o:  
BI_helix.o:  
BI_helix.o:  
BI_helix.o:  
BI_helix.o:  
BI_helix.o:  
BI_helix.o:  
BI_helix.o:  
BI_helix.o:  
BI_helix.o:   
BI_helix.o: $(ARBHOME)/INCLUDE/arbdb.h
BI_helix.o: $(ARBHOME)/INCLUDE/ad_prot.h
BI_helix.o: $(ARBHOME)/INCLUDE/arbdbt.h
BI_helix.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
BI_helix.o: $(ARBHOME)/INCLUDE/aw_root.hxx
BI_helix.o: $(ARBHOME)/INCLUDE/aw_device.hxx
BI_helix.o: $(ARBHOME)/INCLUDE/aw_window.hxx BI_helix.hxx
AWT_filter.o:  
AWT_filter.o:  
AWT_filter.o:  
AWT_filter.o:  
AWT_filter.o:  $(ARBHOME)/INCLUDE/arbdb.h
AWT_filter.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_filter.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_awars.hxx ./awt.hxx
AWT_filter.o: awtfilter.hxx awt_tree.hxx ./awt_pro_a_nucs.hxx
AWT_sel_boxes.o:  
AWT_sel_boxes.o:  
AWT_sel_boxes.o:  
AWT_sel_boxes.o:  
AWT_sel_boxes.o:  
AWT_sel_boxes.o:  
AWT_sel_boxes.o:  
AWT_sel_boxes.o:  
AWT_sel_boxes.o:  
AWT_sel_boxes.o:  
AWT_sel_boxes.o:  
AWT_sel_boxes.o:  
AWT_sel_boxes.o: 
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_sel_boxes.o: $(ARBHOME)/INCLUDE/aw_awars.hxx ./awt.hxx
AWT_sel_boxes.o: awtlocal.hxx
AWT_canio.o:  
AWT_canio.o:  
AWT_canio.o:  
AWT_canio.o:  
AWT_canio.o:  
AWT_canio.o:  
AWT_canio.o:  
AWT_canio.o:  
AWT_canio.o:  
AWT_canio.o:  $(ARBHOME)/INCLUDE/arbdb.h
AWT_canio.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_canio.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_canio.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_canio.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_canio.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_canio.o: $(ARBHOME)/INCLUDE/aw_window.hxx ./awt_canvas.hxx
AWT_canio.o: ./awt.hxx
AWT_nds.o:  
AWT_nds.o:  
AWT_nds.o:  
AWT_nds.o:  
AWT_nds.o:   
AWT_nds.o:  
AWT_nds.o:  
AWT_nds.o:  
AWT_nds.o:   
AWT_nds.o:  $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_nds.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_nds.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_nds.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_nds.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_nds.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_nds.o: $(ARBHOME)/INCLUDE/ad_t_prot.h ./awt.hxx
AWT_nds.o: ./awt_nds.hxx
AWT_query_and_functions.o:  
AWT_query_and_functions.o:  
AWT_query_and_functions.o:  
AWT_query_and_functions.o: 
AWT_query_and_functions.o: 
AWT_query_and_functions.o:  
AWT_query_and_functions.o:  
AWT_query_and_functions.o: 
AWT_query_and_functions.o: 
AWT_query_and_functions.o: 
AWT_query_and_functions.o: 
AWT_query_and_functions.o:  
AWT_query_and_functions.o: 
AWT_query_and_functions.o: 
AWT_query_and_functions.o:  
AWT_query_and_functions.o:  
AWT_query_and_functions.o: 
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_query_and_functions.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_query_and_functions.o: ./awt.hxx awtlocal.hxx
AWT_file_selection.o:  
AWT_file_selection.o:  
AWT_file_selection.o:  
AWT_file_selection.o:  
AWT_file_selection.o:  
AWT_file_selection.o:  
AWT_file_selection.o:  
AWT_file_selection.o:  
AWT_file_selection.o:  
AWT_file_selection.o:  
AWT_file_selection.o:  
AWT_file_selection.o:  
AWT_file_selection.o: 
AWT_file_selection.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_file_selection.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_file_selection.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_file_selection.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_file_selection.o: $(ARBHOME)/INCLUDE/aw_awars.hxx ./awt.hxx
AWT_file_selection.o: awtlocal.hxx 
AWT_file_selection.o: 
AWT_file_selection.o: 
AWT_file_selection.o: 
AWT_file_selection.o:  
AWT_file_selection.o: 
AWT_species_sel_box.o:  
AWT_species_sel_box.o:  
AWT_species_sel_box.o:  
AWT_species_sel_box.o:  
AWT_species_sel_box.o:  
AWT_species_sel_box.o:  
AWT_species_sel_box.o:  
AWT_species_sel_box.o:  
AWT_species_sel_box.o:  
AWT_species_sel_box.o:  
AWT_species_sel_box.o:  
AWT_species_sel_box.o:  
AWT_species_sel_box.o: 
AWT_species_sel_box.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_species_sel_box.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_species_sel_box.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_species_sel_box.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_species_sel_box.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_species_sel_box.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_species_sel_box.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_species_sel_box.o: $(ARBHOME)/INCLUDE/aw_awars.hxx ./awt.hxx
AWT_species_sel_box.o: awtlocal.hxx
AWT_imp_exp.o:  
AWT_imp_exp.o:  
AWT_imp_exp.o:  
AWT_imp_exp.o:  
AWT_imp_exp.o:  $(ARBHOME)/INCLUDE/arbdb.h
AWT_imp_exp.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_imp_exp.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_imp_exp.o: $(ARBHOME)/INCLUDE/ad_t_prot.h ./awt_nds.hxx
AWT_preset.o:  
AWT_preset.o:  
AWT_preset.o:  
AWT_preset.o:  
AWT_preset.o:  
AWT_preset.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_preset.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_preset.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_preset.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_preset.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_preset.o:  
AWT_preset.o:  
AWT_preset.o:  
AWT_preset.o:  
AWT_preset.o:  
AWT_preset.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
AWT_seq_dna.o:  
AWT_seq_dna.o:  
AWT_seq_dna.o:  
AWT_seq_dna.o:  
AWT_seq_dna.o:  
AWT_seq_dna.o:  
AWT_seq_dna.o:  
AWT_seq_dna.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_seq_dna.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_seq_dna.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_seq_dna.o: $(ARBHOME)/INCLUDE/ad_t_prot.h awt_tree.hxx
AWT_seq_dna.o: ./awt_pro_a_nucs.hxx awt_seq_dna.hxx
AWT_seq_protein.o:  
AWT_seq_protein.o:  
AWT_seq_protein.o:  
AWT_seq_protein.o:  
AWT_seq_protein.o:  
AWT_seq_protein.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_seq_protein.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_seq_protein.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_seq_protein.o: $(ARBHOME)/INCLUDE/ad_t_prot.h awt_tree.hxx
AWT_seq_protein.o: ./awt_pro_a_nucs.hxx awt_seq_protein.hxx
AWT_seq_simple_pro.o:  
AWT_seq_simple_pro.o:  
AWT_seq_simple_pro.o:  
AWT_seq_simple_pro.o:  
AWT_seq_simple_pro.o:  
AWT_seq_simple_pro.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_seq_simple_pro.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_seq_simple_pro.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_seq_simple_pro.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_seq_simple_pro.o: awt_tree.hxx ./awt_pro_a_nucs.hxx
AWT_seq_simple_pro.o: awt_seq_simple_pro.hxx
AWT_codon_table.o:  
AWT_codon_table.o:  
AWT_codon_table.o:  
AWT_codon_table.o:  
AWT_codon_table.o:  awt_iupac.hxx
AWT_codon_table.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_codon_table.o:  
AWT_codon_table.o:  
AWT_codon_table.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_codon_table.o: ./awt_codon_table.hxx ./awt.hxx
AWT_iupac.o:  
AWT_iupac.o:  
AWT_iupac.o:  
AWT_iupac.o:   awt_assert.hxx
AWT_iupac.o: awt_iupac.hxx $(ARBHOME)/INCLUDE/arbdb.h
AWT_iupac.o:  
AWT_iupac.o:  
AWT_iupac.o: $(ARBHOME)/INCLUDE/ad_prot.h
