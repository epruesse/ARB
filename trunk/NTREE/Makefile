# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend


CPP_OBJECTS =  NT_edconf.o NT_join.o NT_sort.o NT_cb.o NT_main.o NT_extern.o\
	NT_validNames.o NT_validNameParser.o \
	NT_validManual.o  NT_MAUS.o \
	NT_ins_col.o NT_import.o NT_concatenate.o\
	ad_spec.o ad_trees.o  ad_ali.o ad_ext.o ad_transpro.o \
	AP_consensus.o AP_cprofile.o AP_pos_var_pars.o \
	ETC_check_gcg.o AP_csp_2_gnuplot.o AP_conservProfile2Gnuplot.o

CPP_SOURCES = $(CPP_OBJECTS:.o=.cxx)

DATE_HEADER=nt_date.h

$(MAIN): nt_date_update $(CPP_OBJECTS)
	@$(ARBHOME)/SOURCE_TOOLS/binuptodate.pl $(MAIN) $(DATE_HEADER) $(CPP_OBJECTS) || ( \
		echo "Link $(MAIN)" ; \
		echo $(AR) $(MAIN) $(CPP_OBJECTS) ; \
		$(AR) $(MAIN) $(CPP_OBJECTS) ; \
	)

nt_date.h: nt_date_update

nt_date_update:
	echo '#define DATE' `date '+"%y.%m.%d"'`  >  nt_date.h.tmp
	echo '#define DATE_YEAR' `date '+"%Y"'`   >> nt_date.h.tmp
	../SOURCE_TOOLS/mv_if_diff nt_date.h.tmp nt_date.h

# explicit date dependencies

NT_main.o: nt_date.h

NT_extern.o: nt_date.h


proto:
	rm -f nt_internal.h
	../MAKEBIN/aisc_mkpt -A -E -F NT_,AP_ $(CPP_SOURCES) >nt_internal.h

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

ad_ali.o: $(ARBHOME)/INCLUDE/ad_prot.h
ad_ali.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ad_ali.o: $(ARBHOME)/INCLUDE/arb_assert.h
ad_ali.o: $(ARBHOME)/INCLUDE/arbdb.h
ad_ali.o: $(ARBHOME)/INCLUDE/arbdbt.h
ad_ali.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ad_ali.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ad_ali.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ad_ali.o: $(ARBHOME)/INCLUDE/awt.hxx

ad_ext.o: $(ARBHOME)/INCLUDE/ad_prot.h
ad_ext.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ad_ext.o: $(ARBHOME)/INCLUDE/arb_assert.h
ad_ext.o: $(ARBHOME)/INCLUDE/arbdb.h
ad_ext.o: $(ARBHOME)/INCLUDE/arbdbt.h
ad_ext.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ad_ext.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ad_ext.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ad_ext.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ad_ext.o: $(ARBHOME)/INCLUDE/awt.hxx

ad_spec.o: ad_spec.hxx
ad_spec.o: ntree.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/ad_prot.h
ad_spec.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ad_spec.o: $(ARBHOME)/INCLUDE/arb_assert.h
ad_spec.o: $(ARBHOME)/INCLUDE/arbdb.h
ad_spec.o: $(ARBHOME)/INCLUDE/arbdbt.h
ad_spec.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/awt.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/awt_www.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/awtc_next_neighbours.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/awtlocal.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/probe_design.hxx

ad_transpro.o: $(ARBHOME)/INCLUDE/ad_prot.h
ad_transpro.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ad_transpro.o: $(ARBHOME)/INCLUDE/arb_assert.h
ad_transpro.o: $(ARBHOME)/INCLUDE/arbdb.h
ad_transpro.o: $(ARBHOME)/INCLUDE/arbdbt.h
ad_transpro.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ad_transpro.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ad_transpro.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ad_transpro.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ad_transpro.o: $(ARBHOME)/INCLUDE/awt.hxx
ad_transpro.o: $(ARBHOME)/INCLUDE/awt_codon_table.hxx
ad_transpro.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx

ad_trees.o: $(ARBHOME)/INCLUDE/ad_prot.h
ad_trees.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ad_trees.o: $(ARBHOME)/INCLUDE/arb_assert.h
ad_trees.o: $(ARBHOME)/INCLUDE/arbdb.h
ad_trees.o: $(ARBHOME)/INCLUDE/arbdbt.h
ad_trees.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ad_trees.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ad_trees.o: $(ARBHOME)/INCLUDE/aw_global.hxx
ad_trees.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ad_trees.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ad_trees.o: $(ARBHOME)/INCLUDE/awt.hxx
ad_trees.o: $(ARBHOME)/INCLUDE/awt_imp_exp.hxx
ad_trees.o: $(ARBHOME)/INCLUDE/awt_tree_cmp.hxx
ad_trees.o: $(ARBHOME)/INCLUDE/cat_tree.hxx

AP_consensus.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_consensus.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_consensus.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_consensus.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_consensus.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_consensus.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AP_consensus.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AP_consensus.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AP_consensus.o: $(ARBHOME)/INCLUDE/awt.hxx

AP_conservProfile2Gnuplot.o: ap_conservProfile2Gnuplot.hxx
AP_conservProfile2Gnuplot.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_conservProfile2Gnuplot.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_conservProfile2Gnuplot.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_conservProfile2Gnuplot.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_conservProfile2Gnuplot.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_conservProfile2Gnuplot.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AP_conservProfile2Gnuplot.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AP_conservProfile2Gnuplot.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AP_conservProfile2Gnuplot.o: $(ARBHOME)/INCLUDE/aw_global.hxx
AP_conservProfile2Gnuplot.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AP_conservProfile2Gnuplot.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AP_conservProfile2Gnuplot.o: $(ARBHOME)/INCLUDE/awt.hxx
AP_conservProfile2Gnuplot.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
AP_conservProfile2Gnuplot.o: $(ARBHOME)/INCLUDE/awt_csp.hxx
AP_conservProfile2Gnuplot.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
AP_conservProfile2Gnuplot.o: $(ARBHOME)/INCLUDE/awt_tree.hxx

AP_cprofile.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_cprofile.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_cprofile.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_cprofile.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_cprofile.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_cprofile.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AP_cprofile.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AP_cprofile.o: $(ARBHOME)/INCLUDE/aw_global.hxx
AP_cprofile.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AP_cprofile.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AP_cprofile.o: $(ARBHOME)/INCLUDE/awt.hxx

AP_csp_2_gnuplot.o: ap_csp_2_gnuplot.hxx
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/aw_global.hxx
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/awt.hxx
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/awt_csp.hxx
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/awt_tree.hxx

AP_pos_var_pars.o: ap_pos_var_pars.hxx
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/arb_assert.h
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/awt.hxx
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/awt_tree.hxx

ETC_check_gcg.o: $(ARBHOME)/INCLUDE/ad_prot.h
ETC_check_gcg.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ETC_check_gcg.o: $(ARBHOME)/INCLUDE/arb_assert.h
ETC_check_gcg.o: $(ARBHOME)/INCLUDE/arbdb.h
ETC_check_gcg.o: $(ARBHOME)/INCLUDE/arbdbt.h
ETC_check_gcg.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ETC_check_gcg.o: $(ARBHOME)/INCLUDE/aw_global.hxx
ETC_check_gcg.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ETC_check_gcg.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ETC_check_gcg.o: $(ARBHOME)/INCLUDE/awt.hxx

NT_cb.o: ad_trees.hxx
NT_cb.o: nt_internal.h
NT_cb.o: ntree.hxx
NT_cb.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_cb.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_cb.o: $(ARBHOME)/INCLUDE/arb_assert.h
NT_cb.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_cb.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_cb.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
NT_cb.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
NT_cb.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_cb.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_cb.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_cb.o: $(ARBHOME)/INCLUDE/awt.hxx
NT_cb.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
NT_cb.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
NT_cb.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
NT_cb.o: $(ARBHOME)/INCLUDE/awt_tree.hxx

NT_concatenate.o: nt_concatenate.hxx
NT_concatenate.o: nt_sort.hxx
NT_concatenate.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_concatenate.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_concatenate.o: $(ARBHOME)/INCLUDE/arb_assert.h
NT_concatenate.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_concatenate.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_concatenate.o: $(ARBHOME)/INCLUDE/arbtools.h
NT_concatenate.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
NT_concatenate.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
NT_concatenate.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_concatenate.o: $(ARBHOME)/INCLUDE/aw_question.hxx
NT_concatenate.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_concatenate.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_concatenate.o: $(ARBHOME)/INCLUDE/awt.hxx
NT_concatenate.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
NT_concatenate.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
NT_concatenate.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
NT_concatenate.o: $(ARBHOME)/INCLUDE/awtc_rename.hxx
NT_concatenate.o: $(ARBHOME)/INCLUDE/awtlocal.hxx

NT_edconf.o: nt_edconf.hxx
NT_edconf.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_edconf.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_edconf.o: $(ARBHOME)/INCLUDE/arb_assert.h
NT_edconf.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_edconf.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_edconf.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
NT_edconf.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
NT_edconf.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_edconf.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_edconf.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_edconf.o: $(ARBHOME)/INCLUDE/awt.hxx
NT_edconf.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
NT_edconf.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
NT_edconf.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
NT_edconf.o: $(ARBHOME)/INCLUDE/awt_tree.hxx

NT_extern.o: ad_spec.hxx
NT_extern.o: ad_trees.hxx
NT_extern.o: ap_consensus.hxx
NT_extern.o: ap_conservProfile2Gnuplot.hxx
NT_extern.o: ap_csp_2_gnuplot.hxx
NT_extern.o: ap_pos_var_pars.hxx
NT_extern.o: etc_check_gcg.hxx
NT_extern.o: nt_cb.hxx
NT_extern.o: nt_concatenate.hxx
NT_extern.o: nt_date.h
NT_extern.o: nt_edconf.hxx
NT_extern.o: nt_import.hxx
NT_extern.o: nt_internal.h
NT_extern.o: nt_join.hxx
NT_extern.o: nt_sort.hxx
NT_extern.o: ntree.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_extern.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_extern.o: $(ARBHOME)/INCLUDE/adGene.h
NT_extern.o: $(ARBHOME)/INCLUDE/arb_assert.h
NT_extern.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_extern.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_extern.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/aw_global.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_advice.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_config_manager.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_hotkeys.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_input_mask.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_nds.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_preset.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_tree_cb.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_www.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awtc_rename.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awtc_submission.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awti_export.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/EXP.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/gde.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/GEN.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/primer_design.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/probe_design.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/seq_quality.h
NT_extern.o: $(ARBHOME)/INCLUDE/smartptr.h
NT_extern.o: $(ARBHOME)/INCLUDE/st_window.hxx

NT_import.o: ad_spec.hxx
NT_import.o: nt_import.hxx
NT_import.o: nt_internal.h
NT_import.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_import.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_import.o: $(ARBHOME)/INCLUDE/adGene.h
NT_import.o: $(ARBHOME)/INCLUDE/arb_assert.h
NT_import.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_import.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_import.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_import.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_import.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_import.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
NT_import.o: $(ARBHOME)/INCLUDE/awti_import.hxx
NT_import.o: $(ARBHOME)/INCLUDE/GEN.hxx
NT_import.o: $(ARBHOME)/INCLUDE/mg_merge.hxx

NT_ins_col.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_ins_col.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_ins_col.o: $(ARBHOME)/INCLUDE/arb_assert.h
NT_ins_col.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_ins_col.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_ins_col.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
NT_ins_col.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_ins_col.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_ins_col.o: $(ARBHOME)/INCLUDE/aw_window.hxx

NT_join.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_join.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_join.o: $(ARBHOME)/INCLUDE/arb_assert.h
NT_join.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_join.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_join.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
NT_join.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_join.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_join.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_join.o: $(ARBHOME)/INCLUDE/awt.hxx

NT_main.o: nt_cb.hxx
NT_main.o: nt_concatenate.hxx
NT_main.o: nt_date.h
NT_main.o: ntree.hxx
NT_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_main.o: $(ARBHOME)/INCLUDE/adGene.h
NT_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
NT_main.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_main.o: $(ARBHOME)/INCLUDE/arbtools.h
NT_main.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
NT_main.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
NT_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_main.o: $(ARBHOME)/INCLUDE/aw_global.hxx
NT_main.o: $(ARBHOME)/INCLUDE/aw_question.hxx
NT_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_main.o: $(ARBHOME)/INCLUDE/awt.hxx
NT_main.o: $(ARBHOME)/INCLUDE/awt_advice.hxx
NT_main.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
NT_main.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
NT_main.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
NT_main.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
NT_main.o: $(ARBHOME)/INCLUDE/awti_import.hxx
NT_main.o: $(ARBHOME)/INCLUDE/mg_merge.hxx
NT_main.o: $(ARBHOME)/INCLUDE/seer.hxx
NT_main.o: $(ARBHOME)/INCLUDE/servercntrl.h

NT_MAUS.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_MAUS.o: $(ARBHOME)/INCLUDE/arb_assert.h
NT_MAUS.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_MAUS.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_MAUS.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_MAUS.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_MAUS.o: $(ARBHOME)/INCLUDE/awt.hxx

NT_sort.o: nt_sort.hxx
NT_sort.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_sort.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_sort.o: $(ARBHOME)/INCLUDE/arb_assert.h
NT_sort.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_sort.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_sort.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
NT_sort.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_sort.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_sort.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_sort.o: $(ARBHOME)/INCLUDE/awt.hxx

NT_validManual.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_validManual.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_validManual.o: $(ARBHOME)/INCLUDE/arb_assert.h
NT_validManual.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_validManual.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_validManual.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
NT_validManual.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_validManual.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_validManual.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_validManual.o: $(ARBHOME)/INCLUDE/awt.hxx

NT_validNameParser.o: nt_validNameParser.hxx

NT_validNames.o: nt_validNameParser.hxx
NT_validNames.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_validNames.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_validNames.o: $(ARBHOME)/INCLUDE/arb_assert.h
NT_validNames.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_validNames.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_validNames.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
NT_validNames.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_validNames.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_validNames.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_validNames.o: $(ARBHOME)/INCLUDE/awt.hxx
NT_validNames.o: $(ARBHOME)/INCLUDE/awt_www.hxx
