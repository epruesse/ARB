# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx
CPP_OBJECTS =  PARS_main.o PARS_dtree.o PARS_debug.o PARS_klprops.o AP_buffer.o AP_main.o AP_tree_nlen.o AP_tree_edge.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

.cxx.o: 
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)

# DO NOT DELETE THIS LINE -- make depend depends on it.

PARS_main.o: $(ARBHOME)/INCLUDE/arbdb.h
PARS_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
PARS_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
PARS_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
PARS_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PARS_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PARS_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PARS_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
PARS_main.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
PARS_main.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
PARS_main.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
PARS_main.o: $(ARBHOME)/INCLUDE/awt.hxx
PARS_main.o: $(ARBHOME)/INCLUDE/awt_nds.hxx
PARS_main.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
PARS_main.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
PARS_main.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
PARS_main.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
PARS_main.o: $(ARBHOME)/INCLUDE/awt_tree_cb.hxx
PARS_main.o: $(ARBHOME)/INCLUDE/awt_csp.hxx AP_buffer.hxx
PARS_main.o: parsimony.hxx ap_tree_nlen.hxx 
PARS_main.o: pars_main.hxx pars_dtree.hxx
PARS_dtree.o:  $(ARBHOME)/INCLUDE/arbdb.h
PARS_dtree.o: $(ARBHOME)/INCLUDE/arb_assert.h
PARS_dtree.o: $(ARBHOME)/INCLUDE/ad_prot.h
PARS_dtree.o: $(ARBHOME)/INCLUDE/arbdbt.h
PARS_dtree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PARS_dtree.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PARS_dtree.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PARS_dtree.o: $(ARBHOME)/INCLUDE/aw_window.hxx
PARS_dtree.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
PARS_dtree.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
PARS_dtree.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
PARS_dtree.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
PARS_dtree.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
PARS_dtree.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
PARS_dtree.o: $(ARBHOME)/INCLUDE/awt_seq_protein.hxx
PARS_dtree.o: $(ARBHOME)/INCLUDE/awt_csp.hxx
PARS_dtree.o: $(ARBHOME)/INCLUDE/awt.hxx
PARS_dtree.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx pars_dtree.hxx
PARS_dtree.o: AP_buffer.hxx parsimony.hxx ap_tree_nlen.hxx
PARS_dtree.o: pars_main.hxx pars_debug.hxx
PARS_debug.o: $(ARBHOME)/INCLUDE/arbdb.h
PARS_debug.o: $(ARBHOME)/INCLUDE/arb_assert.h
PARS_debug.o: $(ARBHOME)/INCLUDE/ad_prot.h
PARS_debug.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PARS_debug.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PARS_debug.o: $(ARBHOME)/INCLUDE/aw_window.hxx pars_debug.hxx
PARS_klprops.o: $(ARBHOME)/INCLUDE/arbdb.h
PARS_klprops.o: $(ARBHOME)/INCLUDE/arb_assert.h
PARS_klprops.o: $(ARBHOME)/INCLUDE/ad_prot.h
PARS_klprops.o: $(ARBHOME)/INCLUDE/arbdbt.h
PARS_klprops.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PARS_klprops.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PARS_klprops.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PARS_klprops.o: $(ARBHOME)/INCLUDE/aw_window.hxx
PARS_klprops.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
PARS_klprops.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
PARS_klprops.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
PARS_klprops.o: $(ARBHOME)/INCLUDE/awt.hxx
PARS_klprops.o: $(ARBHOME)/INCLUDE/awt_nds.hxx
PARS_klprops.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
PARS_klprops.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
PARS_klprops.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
PARS_klprops.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
PARS_klprops.o: $(ARBHOME)/INCLUDE/awt_tree_cb.hxx AP_buffer.hxx
PARS_klprops.o: parsimony.hxx ap_tree_nlen.hxx 
PARS_klprops.o: pars_main.hxx pars_dtree.hxx
AP_buffer.o:  $(ARBHOME)/INCLUDE/arbdb.h
AP_buffer.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_buffer.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_buffer.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_buffer.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_buffer.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
AP_buffer.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
AP_buffer.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx AP_buffer.hxx
AP_buffer.o: AP_error.hxx
AP_main.o:  $(ARBHOME)/INCLUDE/arbdb.h
AP_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_main.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
AP_main.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
AP_main.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx AP_buffer.hxx
AP_main.o: parsimony.hxx AP_error.hxx
AP_tree_nlen.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_tree_nlen.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_tree_nlen.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_tree_nlen.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_tree_nlen.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_tree_nlen.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
AP_tree_nlen.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
AP_tree_nlen.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AP_tree_nlen.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx AP_buffer.hxx
AP_tree_nlen.o: parsimony.hxx pars_debug.hxx ap_tree_nlen.hxx
AP_tree_nlen.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AP_tree_edge.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_tree_edge.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_tree_edge.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_tree_edge.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_tree_edge.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_tree_edge.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
AP_tree_edge.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
AP_tree_edge.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx AP_buffer.hxx
AP_tree_edge.o: parsimony.hxx ap_tree_nlen.hxx 
AP_tree_edge.o: $(ARBHOME)/INCLUDE/aw_root.hxx
