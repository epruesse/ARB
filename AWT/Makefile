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


AWT_asciiprint.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_asciiprint.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_asciiprint.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_asciiprint.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_asciiprint.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_asciiprint.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_asciiprint.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_asciiprint.c.o:  
AWT_asciiprint.c.o: 
AWT_asciiprint.c.o:  
AWT_asciiprint.c.o:  
AWT_asciiprint.c.o:  
AWT_asciiprint.c.o:  
AWT_asciiprint.c.o:  
AWT_asciiprint.c.o:  
AWT_asciiprint.c.o:  
AWT_asciiprint.c.o:  
AWT_asciiprint.c.o:  
AWT_asciiprint.c.o: 
AWT_asciiprint.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_asciiprint.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_asciiprint.c.o: awt.hxx awt_asciiprint.hxx
AWT_canio.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_canio.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_canio.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_canio.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_canio.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_canio.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_canio.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_canio.c.o:  
AWT_canio.c.o:  
AWT_canio.c.o:  
AWT_canio.c.o:  
AWT_canio.c.o:  
AWT_canio.c.o:  
AWT_canio.c.o:   
AWT_canio.c.o:  
AWT_canio.c.o:  
AWT_canio.c.o:  
AWT_canio.c.o: 
AWT_canio.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_canio.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_canio.c.o: awt.hxx awt_canvas.hxx
AWT_canvas.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_canvas.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_canvas.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_canvas.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_canvas.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_canvas.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_canvas.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_canvas.c.o:  
AWT_canvas.c.o:  
AWT_canvas.c.o:  
AWT_canvas.c.o:  
AWT_canvas.c.o:  
AWT_canvas.c.o:  
AWT_canvas.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_canvas.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_canvas.c.o: awt_canvas.hxx
AWT_codon_table.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_codon_table.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_codon_table.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_codon_table.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_codon_table.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_codon_table.c.o:  
AWT_codon_table.c.o: 
AWT_codon_table.c.o:  
AWT_codon_table.c.o:  
AWT_codon_table.c.o:  
AWT_codon_table.c.o:  
AWT_codon_table.c.o:  
AWT_codon_table.c.o:  
AWT_codon_table.c.o: 
AWT_codon_table.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_codon_table.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_codon_table.c.o: awt.hxx awt_codon_table.hxx awt_iupac.hxx
AWT_csp.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_csp.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_csp.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_csp.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_csp.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_csp.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_csp.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_csp.c.o:  
AWT_csp.c.o:  
AWT_csp.c.o:  
AWT_csp.c.o:  
AWT_csp.c.o:  
AWT_csp.c.o:  
AWT_csp.c.o:   
AWT_csp.c.o:  
AWT_csp.c.o:  
AWT_csp.c.o:  
AWT_csp.c.o:  
AWT_csp.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_csp.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_csp.c.o: BI_helix.hxx awt.hxx awt_csp.hxx awt_pro_a_nucs.hxx awt_tree.hxx
AWT_dtree.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_dtree.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_dtree.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_dtree.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_dtree.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_dtree.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_dtree.c.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
AWT_dtree.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_dtree.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_dtree.c.o:  
AWT_dtree.c.o:  
AWT_dtree.c.o:  
AWT_dtree.c.o:  
AWT_dtree.c.o:  
AWT_dtree.c.o:  
AWT_dtree.c.o:  
AWT_dtree.c.o:  
AWT_dtree.c.o:  
AWT_dtree.c.o:   
AWT_dtree.c.o:  
AWT_dtree.c.o:  
AWT_dtree.c.o:  
AWT_dtree.c.o:  
AWT_dtree.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
AWT_dtree.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_dtree.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_dtree.c.o: awt.hxx awt_canvas.hxx awt_dtree.hxx awt_nds.hxx
AWT_dtree.c.o: awt_pro_a_nucs.hxx awt_tree.hxx
AWT_file_selection.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_file_selection.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_file_selection.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_file_selection.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_file_selection.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_file_selection.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_file_selection.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_file_selection.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_file_selection.c.o:  
AWT_file_selection.c.o: 
AWT_file_selection.c.o:  
AWT_file_selection.c.o: 
AWT_file_selection.c.o: 
AWT_file_selection.c.o: 
AWT_file_selection.c.o: 
AWT_file_selection.c.o: 
AWT_file_selection.c.o:  
AWT_file_selection.c.o:  
AWT_file_selection.c.o:  
AWT_file_selection.c.o:  
AWT_file_selection.c.o:  
AWT_file_selection.c.o:  
AWT_file_selection.c.o:  
AWT_file_selection.c.o:  
AWT_file_selection.c.o:  
AWT_file_selection.c.o:  
AWT_file_selection.c.o:  
AWT_file_selection.c.o:  
AWT_file_selection.c.o:  
AWT_file_selection.c.o:  
AWT_file_selection.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_file_selection.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_file_selection.c.o: awt.hxx awtlocal.hxx
AWT_filter.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_filter.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_filter.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_filter.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_filter.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_filter.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_filter.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_filter.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_filter.c.o:  
AWT_filter.c.o:  
AWT_filter.c.o:  
AWT_filter.c.o:  
AWT_filter.c.o:  
AWT_filter.c.o:  
AWT_filter.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_filter.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_filter.c.o: awt.hxx awt_pro_a_nucs.hxx awt_tree.hxx awtfilter.hxx
AWT_imp_exp.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_imp_exp.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_imp_exp.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_imp_exp.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_imp_exp.c.o:  
AWT_imp_exp.c.o:  
AWT_imp_exp.c.o:  
AWT_imp_exp.c.o:  
AWT_imp_exp.c.o:  
AWT_imp_exp.c.o:  
AWT_imp_exp.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_imp_exp.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_imp_exp.c.o: awt_nds.hxx
AWT_irstree.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_irstree.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_irstree.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_irstree.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_irstree.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_irstree.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_irstree.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_irstree.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_irstree.c.o:  
AWT_irstree.c.o:  
AWT_irstree.c.o:  
AWT_irstree.c.o:  
AWT_irstree.c.o:  
AWT_irstree.c.o:  
AWT_irstree.c.o:  
AWT_irstree.c.o:  
AWT_irstree.c.o: 
AWT_irstree.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
AWT_irstree.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_irstree.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_irstree.c.o: awt_canvas.hxx awt_dtree.hxx awt_irstree.hxx awt_nds.hxx
AWT_irstree.c.o: awt_pro_a_nucs.hxx awt_tree.hxx
AWT_iupac.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_iupac.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_iupac.c.o:  
AWT_iupac.c.o:  
AWT_iupac.c.o:  
AWT_iupac.c.o:  
AWT_iupac.c.o:  
AWT_iupac.c.o:   
AWT_iupac.c.o:  
AWT_iupac.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_iupac.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_iupac.c.o: awt_assert.hxx awt_iupac.hxx
AWT_map_key.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_map_key.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_map_key.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_map_key.c.o:  
AWT_map_key.c.o:  
AWT_map_key.c.o:  
AWT_map_key.c.o:  
AWT_map_key.c.o:  
AWT_map_key.c.o:  
AWT_map_key.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_map_key.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_map_key.c.o: awt_map_key.hxx
AWT_nds.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_nds.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_nds.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_nds.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_nds.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_nds.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_nds.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_nds.c.o:  
AWT_nds.c.o:  
AWT_nds.c.o:  
AWT_nds.c.o:  
AWT_nds.c.o:  
AWT_nds.c.o:  
AWT_nds.c.o:   
AWT_nds.c.o:   
AWT_nds.c.o:  
AWT_nds.c.o:  
AWT_nds.c.o:  
AWT_nds.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_nds.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h awt.hxx
AWT_nds.c.o: awt_nds.hxx
AWT_nei.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_nei.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_nei.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_nei.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_nei.c.o:  
AWT_nei.c.o:  
AWT_nei.c.o:  
AWT_nei.c.o:  
AWT_nei.c.o:  
AWT_nei.c.o:  
AWT_nei.c.o:   
AWT_nei.c.o:   
AWT_nei.c.o:  
AWT_nei.c.o:  
AWT_nei.c.o:  
AWT_nei.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_nei.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_nei.c.o: awt_nei.hxx awt_pro_a_nucs.hxx awt_tree.hxx
AWT_preset.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_preset.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_preset.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_preset.c.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
AWT_preset.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_preset.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_preset.c.o:  
AWT_preset.c.o:  
AWT_preset.c.o:  
AWT_preset.c.o:  
AWT_preset.c.o:  
AWT_preset.c.o:  
AWT_preset.c.o:  
AWT_preset.c.o:  
AWT_preset.c.o:  
AWT_preset.c.o:  
AWT_preset.c.o:  
AWT_preset.c.o: 
AWT_preset.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_preset.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_pro_a_nucs.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_pro_a_nucs.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_pro_a_nucs.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_pro_a_nucs.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_pro_a_nucs.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_pro_a_nucs.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_pro_a_nucs.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_pro_a_nucs.c.o:  
AWT_pro_a_nucs.c.o: 
AWT_pro_a_nucs.c.o:  
AWT_pro_a_nucs.c.o:  
AWT_pro_a_nucs.c.o:  
AWT_pro_a_nucs.c.o:  
AWT_pro_a_nucs.c.o:  
AWT_pro_a_nucs.c.o:  
AWT_pro_a_nucs.c.o:  
AWT_pro_a_nucs.c.o:  
AWT_pro_a_nucs.c.o:  
AWT_pro_a_nucs.c.o:  
AWT_pro_a_nucs.c.o: 
AWT_pro_a_nucs.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_pro_a_nucs.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_pro_a_nucs.c.o: awt.hxx awt_codon_table.hxx awt_pro_a_nucs.hxx
AWT_query_and_functions.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_query_and_functions.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_query_and_functions.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_query_and_functions.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_query_and_functions.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_query_and_functions.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_query_and_functions.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_query_and_functions.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_query_and_functions.c.o:  
AWT_query_and_functions.c.o: 
AWT_query_and_functions.c.o: 
AWT_query_and_functions.c.o: 
AWT_query_and_functions.c.o: 
AWT_query_and_functions.c.o: 
AWT_query_and_functions.c.o: 
AWT_query_and_functions.c.o: 
AWT_query_and_functions.c.o: 
AWT_query_and_functions.c.o: 
AWT_query_and_functions.c.o: 
AWT_query_and_functions.c.o: 
AWT_query_and_functions.c.o: 
AWT_query_and_functions.c.o:  
AWT_query_and_functions.c.o:  
AWT_query_and_functions.c.o:  
AWT_query_and_functions.c.o:  
AWT_query_and_functions.c.o:  
AWT_query_and_functions.c.o: 
AWT_query_and_functions.c.o: 
AWT_query_and_functions.c.o: 
AWT_query_and_functions.c.o:  
AWT_query_and_functions.c.o:  
AWT_query_and_functions.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_query_and_functions.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_query_and_functions.c.o: awt.hxx awtlocal.hxx
AWT_sel_boxes.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_sel_boxes.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_sel_boxes.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_sel_boxes.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_sel_boxes.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_sel_boxes.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_sel_boxes.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_sel_boxes.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_sel_boxes.c.o:  
AWT_sel_boxes.c.o:  
AWT_sel_boxes.c.o: 
AWT_sel_boxes.c.o: 
AWT_sel_boxes.c.o:  
AWT_sel_boxes.c.o:  
AWT_sel_boxes.c.o:  
AWT_sel_boxes.c.o:  
AWT_sel_boxes.c.o:  
AWT_sel_boxes.c.o:  
AWT_sel_boxes.c.o:  
AWT_sel_boxes.c.o:  
AWT_sel_boxes.c.o:  
AWT_sel_boxes.c.o:  
AWT_sel_boxes.c.o:  
AWT_sel_boxes.c.o:  
AWT_sel_boxes.c.o: 
AWT_sel_boxes.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_sel_boxes.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_sel_boxes.c.o: awt.hxx awtlocal.hxx
AWT_seq_colors.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_seq_colors.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_seq_colors.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_seq_colors.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_seq_colors.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_seq_colors.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_seq_colors.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_seq_colors.c.o:  
AWT_seq_colors.c.o: 
AWT_seq_colors.c.o:  
AWT_seq_colors.c.o:  
AWT_seq_colors.c.o:  
AWT_seq_colors.c.o:  
AWT_seq_colors.c.o:  
AWT_seq_colors.c.o:  
AWT_seq_colors.c.o:  
AWT_seq_colors.c.o:  
AWT_seq_colors.c.o:  
AWT_seq_colors.c.o:  
AWT_seq_colors.c.o: 
AWT_seq_colors.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_seq_colors.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_seq_colors.c.o: awt.hxx awt_seq_colors.hxx
AWT_seq_dna.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_seq_dna.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_seq_dna.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_seq_dna.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_seq_dna.c.o:  
AWT_seq_dna.c.o:  
AWT_seq_dna.c.o:  
AWT_seq_dna.c.o:  
AWT_seq_dna.c.o:  
AWT_seq_dna.c.o:  
AWT_seq_dna.c.o:  
AWT_seq_dna.c.o:  
AWT_seq_dna.c.o:  
AWT_seq_dna.c.o:  
AWT_seq_dna.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
AWT_seq_dna.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_seq_dna.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_seq_dna.c.o: awt_pro_a_nucs.hxx awt_seq_dna.hxx awt_tree.hxx
AWT_seq_protein.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_seq_protein.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_seq_protein.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_seq_protein.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_seq_protein.c.o:  
AWT_seq_protein.c.o:  
AWT_seq_protein.c.o:  
AWT_seq_protein.c.o:  
AWT_seq_protein.c.o:  
AWT_seq_protein.c.o:  
AWT_seq_protein.c.o: 
AWT_seq_protein.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_seq_protein.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_seq_protein.c.o: awt_pro_a_nucs.hxx awt_seq_protein.hxx awt_tree.hxx
AWT_seq_simple_pro.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_seq_simple_pro.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_seq_simple_pro.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_seq_simple_pro.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_seq_simple_pro.c.o: 
AWT_seq_simple_pro.c.o: 
AWT_seq_simple_pro.c.o: 
AWT_seq_simple_pro.c.o: 
AWT_seq_simple_pro.c.o:  
AWT_seq_simple_pro.c.o:  
AWT_seq_simple_pro.c.o:  
AWT_seq_simple_pro.c.o:  
AWT_seq_simple_pro.c.o: 
AWT_seq_simple_pro.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_seq_simple_pro.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_seq_simple_pro.c.o: awt_pro_a_nucs.hxx awt_seq_simple_pro.hxx awt_tree.hxx
AWT_species_sel_box.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_species_sel_box.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_species_sel_box.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_species_sel_box.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_species_sel_box.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_species_sel_box.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_species_sel_box.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_species_sel_box.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_species_sel_box.c.o:  
AWT_species_sel_box.c.o: 
AWT_species_sel_box.c.o: 
AWT_species_sel_box.c.o: 
AWT_species_sel_box.c.o: 
AWT_species_sel_box.c.o: 
AWT_species_sel_box.c.o:  
AWT_species_sel_box.c.o: 
AWT_species_sel_box.c.o: 
AWT_species_sel_box.c.o:  
AWT_species_sel_box.c.o:  
AWT_species_sel_box.c.o:  
AWT_species_sel_box.c.o:  
AWT_species_sel_box.c.o:  
AWT_species_sel_box.c.o:  
AWT_species_sel_box.c.o:  
AWT_species_sel_box.c.o: 
AWT_species_sel_box.c.o:  
AWT_species_sel_box.c.o:  
AWT_species_sel_box.c.o: 
AWT_species_sel_box.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_species_sel_box.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_species_sel_box.c.o: awt.hxx awtlocal.hxx
AWT_tables.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_tables.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_tables.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_tables.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_tables.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_tables.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_tables.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_tables.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_tables.c.o:  
AWT_tables.c.o:  
AWT_tables.c.o:  
AWT_tables.c.o:  
AWT_tables.c.o:  
AWT_tables.c.o:  
AWT_tables.c.o:  
AWT_tables.c.o:  
AWT_tables.c.o:  
AWT_tables.c.o:  
AWT_tables.c.o:  
AWT_tables.c.o:  
AWT_tables.c.o:  
AWT_tables.c.o:  
AWT_tables.c.o:  
AWT_tables.c.o: 
AWT_tables.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_tables.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_tables.c.o: awt.hxx awtlocal.hxx
AWT_tree.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_tree.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_tree.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_tree.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_tree.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_tree.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_tree.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_tree.c.o:  
AWT_tree.c.o:  
AWT_tree.c.o:  
AWT_tree.c.o:  
AWT_tree.c.o:  
AWT_tree.c.o:  
AWT_tree.c.o:  
AWT_tree.c.o:  
AWT_tree.c.o:   
AWT_tree.c.o:   
AWT_tree.c.o:  
AWT_tree.c.o:  
AWT_tree.c.o:  
AWT_tree.c.o: 
AWT_tree.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
AWT_tree.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_tree.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_tree.c.o: awt_canvas.hxx awt_pro_a_nucs.hxx awt_tree.hxx
AWT_tree_cb.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_tree_cb.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_tree_cb.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_tree_cb.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_tree_cb.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_tree_cb.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_tree_cb.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_tree_cb.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_tree_cb.c.o:  
AWT_tree_cb.c.o:  
AWT_tree_cb.c.o:  
AWT_tree_cb.c.o:  
AWT_tree_cb.c.o:  
AWT_tree_cb.c.o:  
AWT_tree_cb.c.o:  
AWT_tree_cb.c.o:  
AWT_tree_cb.c.o:  
AWT_tree_cb.c.o:  
AWT_tree_cb.c.o: 
AWT_tree_cb.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_tree_cb.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_tree_cb.c.o: awt_canvas.hxx awt_dtree.hxx awt_pro_a_nucs.hxx awt_tree.hxx
AWT_tree_cb.c.o: awt_tree_cb.hxx
AWT_tree_cmp.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_tree_cmp.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_tree_cmp.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_tree_cmp.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_tree_cmp.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_tree_cmp.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_tree_cmp.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_tree_cmp.c.o:  
AWT_tree_cmp.c.o:  
AWT_tree_cmp.c.o:  
AWT_tree_cmp.c.o:  
AWT_tree_cmp.c.o:  
AWT_tree_cmp.c.o:  
AWT_tree_cmp.c.o:  
AWT_tree_cmp.c.o:  
AWT_tree_cmp.c.o: /usr/include/g++/cctype /usr/include/g++/cstddef
AWT_tree_cmp.c.o: /usr/include/g++/cstring 
AWT_tree_cmp.c.o: /usr/include/g++/iterator /usr/include/g++/std/bastring.cc
AWT_tree_cmp.c.o: 
AWT_tree_cmp.c.o:  
AWT_tree_cmp.c.o: 
AWT_tree_cmp.c.o: 
AWT_tree_cmp.c.o:  
AWT_tree_cmp.c.o: /usr/include/g++/string 
AWT_tree_cmp.c.o:  
AWT_tree_cmp.c.o:  
AWT_tree_cmp.c.o:  
AWT_tree_cmp.c.o:  
AWT_tree_cmp.c.o:  
AWT_tree_cmp.c.o:  
AWT_tree_cmp.c.o:  
AWT_tree_cmp.c.o: 
AWT_tree_cmp.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_tree_cmp.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AWT_tree_cmp.c.o: awt_pro_a_nucs.hxx awt_tree.hxx awt_tree_cmp.hxx
AWT_www.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_www.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_www.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_www.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_www.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_www.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_www.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_www.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_www.c.o:  
AWT_www.c.o:  
AWT_www.c.o:  
AWT_www.c.o:  
AWT_www.c.o:  
AWT_www.c.o:  
AWT_www.c.o:   
AWT_www.c.o:   
AWT_www.c.o:  
AWT_www.c.o:  
AWT_www.c.o:  
AWT_www.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AWT_www.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h awt.hxx
BI_helix.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
BI_helix.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
BI_helix.c.o: $(ARBHOME)/INCLUDE/arbdb.h
BI_helix.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
BI_helix.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
BI_helix.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
BI_helix.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
BI_helix.c.o:  
BI_helix.c.o:  
BI_helix.c.o:  
BI_helix.c.o:  
BI_helix.c.o:  
BI_helix.c.o:  
BI_helix.c.o:  
BI_helix.c.o:   
BI_helix.c.o:  
BI_helix.c.o:  
BI_helix.c.o:  
BI_helix.c.o: 
BI_helix.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
BI_helix.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
BI_helix.c.o: BI_helix.hxx
