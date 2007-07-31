# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

CPP_OBJECTS = DI_mldist.o DI_protdist.o DI_compress_matrix.o DI_view_matrix.o DI_save_matr.o DI_main.o DI_matr.o distanalyse.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)
.cxx.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)

clean:
	rm -f $(CPP_OBJECTS) *.a

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

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

DI_compress_matrix.o: di_matr.hxx
DI_compress_matrix.o: di_view_matrix.hxx
DI_compress_matrix.o: dist.hxx
DI_compress_matrix.o: $(ARBHOME)/INCLUDE/ad_prot.h
DI_compress_matrix.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
DI_compress_matrix.o: $(ARBHOME)/INCLUDE/arb_assert.h
DI_compress_matrix.o: $(ARBHOME)/INCLUDE/arbdb.h
DI_compress_matrix.o: $(ARBHOME)/INCLUDE/arbdbt.h
DI_compress_matrix.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
DI_compress_matrix.o: $(ARBHOME)/INCLUDE/aw_device.hxx
DI_compress_matrix.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
DI_compress_matrix.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
DI_compress_matrix.o: $(ARBHOME)/INCLUDE/aw_root.hxx
DI_compress_matrix.o: $(ARBHOME)/INCLUDE/aw_window.hxx
DI_compress_matrix.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
DI_compress_matrix.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
DI_compress_matrix.o: $(ARBHOME)/INCLUDE/awt_seq_simple_pro.hxx
DI_compress_matrix.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
DI_compress_matrix.o: $(ARBHOME)/INCLUDE/BI_helix.hxx

DI_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
DI_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
DI_main.o: $(ARBHOME)/INCLUDE/arbdb.h
DI_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
DI_main.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
DI_main.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
DI_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
DI_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
DI_main.o: $(ARBHOME)/INCLUDE/awt.hxx
DI_main.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
DI_main.o: $(ARBHOME)/INCLUDE/servercntrl.h

DI_matr.o: di_matr.hxx
DI_matr.o: di_protdist.hxx
DI_matr.o: di_view_matrix.hxx
DI_matr.o: dist.hxx
DI_matr.o: $(ARBHOME)/INCLUDE/ad_prot.h
DI_matr.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
DI_matr.o: $(ARBHOME)/INCLUDE/arb_assert.h
DI_matr.o: $(ARBHOME)/INCLUDE/arbdb.h
DI_matr.o: $(ARBHOME)/INCLUDE/arbdbt.h
DI_matr.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
DI_matr.o: $(ARBHOME)/INCLUDE/aw_device.hxx
DI_matr.o: $(ARBHOME)/INCLUDE/aw_global.hxx
DI_matr.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
DI_matr.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
DI_matr.o: $(ARBHOME)/INCLUDE/aw_root.hxx
DI_matr.o: $(ARBHOME)/INCLUDE/aw_window.hxx
DI_matr.o: $(ARBHOME)/INCLUDE/awt.hxx
DI_matr.o: $(ARBHOME)/INCLUDE/awt_csp.hxx
DI_matr.o: $(ARBHOME)/INCLUDE/awt_macro.hxx
DI_matr.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
DI_matr.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
DI_matr.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
DI_matr.o: $(ARBHOME)/INCLUDE/awt_seq_simple_pro.hxx
DI_matr.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
DI_matr.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
DI_matr.o: $(ARBHOME)/INCLUDE/CT_ctree.hxx

DI_mldist.o: di_matr.hxx
DI_mldist.o: di_mldist.hxx
DI_mldist.o: dist.hxx
DI_mldist.o: $(ARBHOME)/INCLUDE/ad_prot.h
DI_mldist.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
DI_mldist.o: $(ARBHOME)/INCLUDE/arb_assert.h
DI_mldist.o: $(ARBHOME)/INCLUDE/arbdb.h
DI_mldist.o: $(ARBHOME)/INCLUDE/arbdbt.h
DI_mldist.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
DI_mldist.o: $(ARBHOME)/INCLUDE/aw_device.hxx
DI_mldist.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
DI_mldist.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
DI_mldist.o: $(ARBHOME)/INCLUDE/aw_root.hxx
DI_mldist.o: $(ARBHOME)/INCLUDE/aw_window.hxx
DI_mldist.o: $(ARBHOME)/INCLUDE/awt.hxx
DI_mldist.o: $(ARBHOME)/INCLUDE/awt_csp.hxx
DI_mldist.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
DI_mldist.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
DI_mldist.o: $(ARBHOME)/INCLUDE/awt_seq_simple_pro.hxx
DI_mldist.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
DI_mldist.o: $(ARBHOME)/INCLUDE/BI_helix.hxx

DI_protdist.o: di_matr.hxx
DI_protdist.o: di_protdist.hxx
DI_protdist.o: dist.hxx
DI_protdist.o: $(ARBHOME)/INCLUDE/ad_prot.h
DI_protdist.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
DI_protdist.o: $(ARBHOME)/INCLUDE/arb_assert.h
DI_protdist.o: $(ARBHOME)/INCLUDE/arbdb.h
DI_protdist.o: $(ARBHOME)/INCLUDE/arbdbt.h
DI_protdist.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
DI_protdist.o: $(ARBHOME)/INCLUDE/aw_device.hxx
DI_protdist.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
DI_protdist.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
DI_protdist.o: $(ARBHOME)/INCLUDE/aw_root.hxx
DI_protdist.o: $(ARBHOME)/INCLUDE/aw_window.hxx
DI_protdist.o: $(ARBHOME)/INCLUDE/awt.hxx
DI_protdist.o: $(ARBHOME)/INCLUDE/awt_csp.hxx
DI_protdist.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
DI_protdist.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
DI_protdist.o: $(ARBHOME)/INCLUDE/awt_seq_simple_pro.hxx
DI_protdist.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
DI_protdist.o: $(ARBHOME)/INCLUDE/BI_helix.hxx

DI_save_matr.o: di_matr.hxx
DI_save_matr.o: dist.hxx
DI_save_matr.o: $(ARBHOME)/INCLUDE/ad_prot.h
DI_save_matr.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
DI_save_matr.o: $(ARBHOME)/INCLUDE/arb_assert.h
DI_save_matr.o: $(ARBHOME)/INCLUDE/arbdb.h
DI_save_matr.o: $(ARBHOME)/INCLUDE/arbdbt.h
DI_save_matr.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
DI_save_matr.o: $(ARBHOME)/INCLUDE/aw_device.hxx
DI_save_matr.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
DI_save_matr.o: $(ARBHOME)/INCLUDE/aw_root.hxx
DI_save_matr.o: $(ARBHOME)/INCLUDE/aw_window.hxx
DI_save_matr.o: $(ARBHOME)/INCLUDE/awt_csp.hxx
DI_save_matr.o: $(ARBHOME)/INCLUDE/awt_nds.hxx
DI_save_matr.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
DI_save_matr.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
DI_save_matr.o: $(ARBHOME)/INCLUDE/awt_seq_simple_pro.hxx
DI_save_matr.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
DI_save_matr.o: $(ARBHOME)/INCLUDE/BI_helix.hxx

DI_view_matrix.o: di_matr.hxx
DI_view_matrix.o: di_view_matrix.hxx
DI_view_matrix.o: dist.hxx
DI_view_matrix.o: $(ARBHOME)/INCLUDE/ad_prot.h
DI_view_matrix.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
DI_view_matrix.o: $(ARBHOME)/INCLUDE/arb_assert.h
DI_view_matrix.o: $(ARBHOME)/INCLUDE/arbdb.h
DI_view_matrix.o: $(ARBHOME)/INCLUDE/arbdbt.h
DI_view_matrix.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
DI_view_matrix.o: $(ARBHOME)/INCLUDE/aw_device.hxx
DI_view_matrix.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
DI_view_matrix.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
DI_view_matrix.o: $(ARBHOME)/INCLUDE/aw_root.hxx
DI_view_matrix.o: $(ARBHOME)/INCLUDE/aw_window.hxx
DI_view_matrix.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
DI_view_matrix.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
DI_view_matrix.o: $(ARBHOME)/INCLUDE/awt_seq_simple_pro.hxx
DI_view_matrix.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
DI_view_matrix.o: $(ARBHOME)/INCLUDE/BI_helix.hxx

distanalyse.o: di_matr.hxx
distanalyse.o: dist.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/ad_prot.h
distanalyse.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
distanalyse.o: $(ARBHOME)/INCLUDE/arb_assert.h
distanalyse.o: $(ARBHOME)/INCLUDE/arbdb.h
distanalyse.o: $(ARBHOME)/INCLUDE/arbdbt.h
distanalyse.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/aw_device.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/aw_root.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/aw_window.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/awt.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/awt_seq_simple_pro.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
distanalyse.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
