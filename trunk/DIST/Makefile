# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx

CPP_OBJECTS = PH_mldist.o PH_protdist.o PH_compress_matrix.o PH_view_matrix.o PH_save_matr.o PH_main.o PH_matr.o distanalyse.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)
.cxx.o: 
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)
depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)
# DO NOT DELETE THIS LINE -- make depend depends on it.

PH_mldist.o:  $(ARBHOME)/INCLUDE/arbdb.h
PH_mldist.o: $(ARBHOME)/INCLUDE/arb_assert.h
PH_mldist.o: $(ARBHOME)/INCLUDE/ad_prot.h
PH_mldist.o: $(ARBHOME)/INCLUDE/arbdbt.h
PH_mldist.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PH_mldist.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PH_mldist.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PH_mldist.o: $(ARBHOME)/INCLUDE/aw_window.hxx
PH_mldist.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
PH_mldist.o: $(ARBHOME)/INCLUDE/awt.hxx
PH_mldist.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
PH_mldist.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
PH_mldist.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx dist.hxx
PH_mldist.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
PH_mldist.o: $(ARBHOME)/INCLUDE/awt_seq_simple_pro.hxx
PH_mldist.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
PH_mldist.o: $(ARBHOME)/INCLUDE/awt_csp.hxx ph_matr.hxx ph_mldist.hxx
PH_protdist.o: $(ARBHOME)/INCLUDE/arbdb.h
PH_protdist.o: $(ARBHOME)/INCLUDE/arb_assert.h
PH_protdist.o: $(ARBHOME)/INCLUDE/ad_prot.h
PH_protdist.o: $(ARBHOME)/INCLUDE/arbdbt.h
PH_protdist.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PH_protdist.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PH_protdist.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PH_protdist.o: $(ARBHOME)/INCLUDE/aw_window.hxx
PH_protdist.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
PH_protdist.o: $(ARBHOME)/INCLUDE/awt.hxx
PH_protdist.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
PH_protdist.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
PH_protdist.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx dist.hxx
PH_protdist.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
PH_protdist.o: $(ARBHOME)/INCLUDE/awt_seq_simple_pro.hxx
PH_protdist.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
PH_protdist.o: $(ARBHOME)/INCLUDE/awt_csp.hxx ph_matr.hxx
PH_protdist.o: ph_protdist.hxx
PH_compress_matrix.o: $(ARBHOME)/INCLUDE/arbdb.h
PH_compress_matrix.o: $(ARBHOME)/INCLUDE/arb_assert.h
PH_compress_matrix.o: $(ARBHOME)/INCLUDE/ad_prot.h
PH_compress_matrix.o: $(ARBHOME)/INCLUDE/arbdbt.h
PH_compress_matrix.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PH_compress_matrix.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PH_compress_matrix.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PH_compress_matrix.o: $(ARBHOME)/INCLUDE/aw_window.hxx
PH_compress_matrix.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
PH_compress_matrix.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
PH_compress_matrix.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
PH_compress_matrix.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx dist.hxx
PH_compress_matrix.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
PH_compress_matrix.o: $(ARBHOME)/INCLUDE/awt_seq_simple_pro.hxx
PH_compress_matrix.o: $(ARBHOME)/INCLUDE/BI_helix.hxx ph_matr.hxx
PH_compress_matrix.o: ./ph_view_matrix.hxx
PH_view_matrix.o: $(ARBHOME)/INCLUDE/arbdb.h
PH_view_matrix.o: $(ARBHOME)/INCLUDE/arb_assert.h
PH_view_matrix.o: $(ARBHOME)/INCLUDE/ad_prot.h
PH_view_matrix.o: $(ARBHOME)/INCLUDE/arbdbt.h
PH_view_matrix.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PH_view_matrix.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PH_view_matrix.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PH_view_matrix.o: $(ARBHOME)/INCLUDE/aw_window.hxx
PH_view_matrix.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
PH_view_matrix.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
PH_view_matrix.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
PH_view_matrix.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx dist.hxx
PH_view_matrix.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
PH_view_matrix.o: $(ARBHOME)/INCLUDE/awt_seq_simple_pro.hxx
PH_view_matrix.o: $(ARBHOME)/INCLUDE/BI_helix.hxx ph_matr.hxx
PH_view_matrix.o: ./ph_view_matrix.hxx
PH_save_matr.o: $(ARBHOME)/INCLUDE/arbdb.h
PH_save_matr.o: $(ARBHOME)/INCLUDE/arb_assert.h
PH_save_matr.o: $(ARBHOME)/INCLUDE/ad_prot.h
PH_save_matr.o: $(ARBHOME)/INCLUDE/arbdbt.h
PH_save_matr.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PH_save_matr.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PH_save_matr.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PH_save_matr.o: $(ARBHOME)/INCLUDE/aw_window.hxx
PH_save_matr.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
PH_save_matr.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
PH_save_matr.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx dist.hxx
PH_save_matr.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
PH_save_matr.o: $(ARBHOME)/INCLUDE/awt_seq_simple_pro.hxx
PH_save_matr.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
PH_save_matr.o: $(ARBHOME)/INCLUDE/awt_nds.hxx
PH_save_matr.o: $(ARBHOME)/INCLUDE/awt_csp.hxx ph_matr.hxx
PH_main.o: $(ARBHOME)/INCLUDE/arbdb.h
PH_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
PH_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
PH_main.o: $(ARBHOME)/INCLUDE/servercntrl.h
PH_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
PH_main.o: $(ARBHOME)/INCLUDE/awt.hxx
PH_main.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
PH_matr.o:  $(ARBHOME)/INCLUDE/arbdb.h
PH_matr.o: $(ARBHOME)/INCLUDE/arb_assert.h
PH_matr.o: $(ARBHOME)/INCLUDE/ad_prot.h
PH_matr.o: $(ARBHOME)/INCLUDE/arbdbt.h
PH_matr.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PH_matr.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/aw_window.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/awt.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx dist.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/awt_seq_simple_pro.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/awt_csp.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/CT_ctree.hxx ph_matr.hxx ph_protdist.hxx
PH_matr.o: ./ph_view_matrix.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/arbdb.h
distanalyse.o: $(ARBHOME)/INCLUDE/arb_assert.h
distanalyse.o: $(ARBHOME)/INCLUDE/ad_prot.h
distanalyse.o: $(ARBHOME)/INCLUDE/arbdbt.h
distanalyse.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
distanalyse.o: $(ARBHOME)/INCLUDE/aw_root.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/aw_device.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/aw_window.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/awt.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx dist.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/awt_seq_simple_pro.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/BI_helix.hxx ph_matr.hxx
