# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx


CPP_OBJECTS =  NT_edconf.o NT_join.o NT_sort.o NT_cb.o NT_main.o NT_extern.o \
	NT_ins_col.o NT_import.o\
	ad_spec.o ad_trees.o  ad_ali.o ad_ext.o ad_transpro.o \
	AP_consensus.o AP_cprofile.o AP_pos_var_pars.o \
	ETC_check_gcg.o AP_csp_2_gnuplot.o

CPP_SOURCES = $(CPP_OBJECTS:.o=.cxx)

$(MAIN): nt_date_dummy $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

nt_date.h: $(CPP_SOURCES)
	echo '#define DATE' `date '+"%y_%m_%d"'`  >nt_date.h

nt_date_dummy:
	$(MAKE) nt_date.h

proto:
	rm -f nt_internal.h
	../MAKEBIN/aisc_mkpt -A -E -F NT_,AP_ $(CPP_OBJECTS:.o=.cxx) >nt_internal.h

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)
# DO NOT DELETE THIS LINE -- make depend depends on it.

NT_edconf.o:  
NT_edconf.o:  
NT_edconf.o:  
NT_edconf.o:  
NT_edconf.o:  
NT_edconf.o:  
NT_edconf.o:  
NT_edconf.o:  
NT_edconf.o:  
NT_edconf.o:  
NT_edconf.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_edconf.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_edconf.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_edconf.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_edconf.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
NT_edconf.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_edconf.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_edconf.o: $(ARBHOME)/INCLUDE/aw_window.hxx nt_edconf.hxx
NT_edconf.o: $(ARBHOME)/INCLUDE/awt.hxx
NT_edconf.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
NT_edconf.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
NT_edconf.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
NT_edconf.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
NT_join.o:  
NT_join.o:  
NT_join.o:  
NT_join.o:  
NT_join.o:  $(ARBHOME)/INCLUDE/arbdb.h
NT_join.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_join.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_join.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_join.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_join.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_join.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_join.o: $(ARBHOME)/INCLUDE/awt.hxx
NT_sort.o:  
NT_sort.o:  
NT_sort.o:  
NT_sort.o:  
NT_sort.o:   
NT_sort.o:  
NT_sort.o:  
NT_sort.o:  
NT_sort.o:   
NT_sort.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_sort.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_sort.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_sort.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_sort.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_sort.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_sort.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_sort.o: $(ARBHOME)/INCLUDE/awt.hxx nt_sort.hxx
NT_cb.o:  
NT_cb.o:  
NT_cb.o:  
NT_cb.o:  
NT_cb.o:   
NT_cb.o:  
NT_cb.o:  
NT_cb.o:  
NT_cb.o:  $(ARBHOME)/INCLUDE/arbdb.h
NT_cb.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_cb.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_cb.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_cb.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_cb.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_cb.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_cb.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
NT_cb.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
NT_cb.o: $(ARBHOME)/INCLUDE/awt.hxx
NT_cb.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
NT_cb.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
NT_cb.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx ad_trees.hxx ntree.hxx
NT_main.o:  
NT_main.o:  
NT_main.o:  
NT_main.o:  
NT_main.o:  
NT_main.o:   
NT_main.o:  
NT_main.o:  
NT_main.o:  
NT_main.o:  $(ARBHOME)/INCLUDE/arbdb.h
NT_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_main.o: $(ARBHOME)/INCLUDE/servercntrl.h
NT_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_main.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
NT_main.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
NT_main.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
NT_main.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
NT_main.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
NT_main.o: $(ARBHOME)/INCLUDE/awt.hxx
NT_main.o: $(ARBHOME)/INCLUDE/awti_import.hxx ntree.hxx
NT_main.o: nt_cb.hxx $(ARBHOME)/INCLUDE/mg_merge.hxx
NT_main.o: $(ARBHOME)/INCLUDE/seer.hxx
NT_extern.o:  
NT_extern.o:  
NT_extern.o:  
NT_extern.o:  
NT_extern.o:  
NT_extern.o:  
NT_extern.o:  
NT_extern.o:  
NT_extern.o:  
NT_extern.o:  $(ARBHOME)/INCLUDE/arbdb.h
NT_extern.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_extern.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_extern.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_extern.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/aw_awars.hxx ad_trees.hxx
NT_extern.o: ad_ext.hxx ad_spec.hxx $(ARBHOME)/INCLUDE/awt.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_nds.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_preset.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awtc_rename.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awtc_submission.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/gde.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_www.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awt_tree_cb.hxx ntree.hxx
NT_extern.o: nt_cb.hxx etc_check_gcg.hxx nt_sort.hxx ap_consensus.hxx
NT_extern.o: ap_csp_2_gnuplot.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/awti_export.hxx nt_join.hxx
NT_extern.o: nt_edconf.hxx ap_pos_var_pars.hxx ad_ali.hxx nt_import.hxx
NT_extern.o: nt_date.h $(ARBHOME)/INCLUDE/st_window.hxx
NT_extern.o: $(ARBHOME)/INCLUDE/probe_design.hxx
NT_ins_col.o:  
NT_ins_col.o:  
NT_ins_col.o:  
NT_ins_col.o:  
NT_ins_col.o:  
NT_ins_col.o:  
NT_ins_col.o:  
NT_ins_col.o:  
NT_ins_col.o:  
NT_ins_col.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_ins_col.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_ins_col.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_ins_col.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_ins_col.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_ins_col.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_ins_col.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_ins_col.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
NT_import.o:  
NT_import.o:  
NT_import.o:  
NT_import.o:  
NT_import.o:  
NT_import.o:  
NT_import.o:  
NT_import.o:  
NT_import.o:  
NT_import.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_import.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_import.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_import.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_import.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_import.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_import.o: $(ARBHOME)/INCLUDE/awti_import.hxx
NT_import.o: $(ARBHOME)/INCLUDE/mg_merge.hxx nt_import.hxx
NT_import.o: ad_spec.hxx $(ARBHOME)/INCLUDE/GEN.hxx
ad_spec.o:  
ad_spec.o:  
ad_spec.o:  
ad_spec.o:  
ad_spec.o:  $(ARBHOME)/INCLUDE/arbdb.h
ad_spec.o: $(ARBHOME)/INCLUDE/ad_prot.h
ad_spec.o: $(ARBHOME)/INCLUDE/arbdbt.h
ad_spec.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ad_spec.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/aw_awars.hxx ad_spec.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/awt.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/awt_www.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/awtlocal.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/awtc_next_neighbours.hxx ntree.hxx
ad_spec.o: $(ARBHOME)/INCLUDE/probe_design.hxx
ad_trees.o:  
ad_trees.o:  
ad_trees.o:  
ad_trees.o:  
ad_trees.o:  
ad_trees.o:  
ad_trees.o:  
ad_trees.o:  
ad_trees.o:  
ad_trees.o:   
ad_trees.o: $(ARBHOME)/INCLUDE/arbdb.h
ad_trees.o: $(ARBHOME)/INCLUDE/ad_prot.h
ad_trees.o: $(ARBHOME)/INCLUDE/arbdbt.h
ad_trees.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ad_trees.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ad_trees.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ad_trees.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ad_trees.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ad_trees.o: $(ARBHOME)/INCLUDE/awt.hxx
ad_trees.o: $(ARBHOME)/INCLUDE/awt_imp_exp.hxx
ad_trees.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ad_trees.o: $(ARBHOME)/INCLUDE/awt_tree_cmp.hxx
ad_ali.o:  
ad_ali.o:  
ad_ali.o:  
ad_ali.o:  
ad_ali.o:   
ad_ali.o:  
ad_ali.o:  
ad_ali.o:  
ad_ali.o:   
ad_ali.o:  $(ARBHOME)/INCLUDE/aw_root.hxx
ad_ali.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ad_ali.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ad_ali.o: $(ARBHOME)/INCLUDE/arbdb.h
ad_ali.o: $(ARBHOME)/INCLUDE/ad_prot.h
ad_ali.o: $(ARBHOME)/INCLUDE/arbdbt.h
ad_ali.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ad_ali.o: $(ARBHOME)/INCLUDE/awt.hxx
ad_ext.o:  
ad_ext.o:  
ad_ext.o:  
ad_ext.o:  
ad_ext.o:   
ad_ext.o: $(ARBHOME)/INCLUDE/arbdb.h
ad_ext.o: $(ARBHOME)/INCLUDE/ad_prot.h
ad_ext.o: $(ARBHOME)/INCLUDE/arbdbt.h
ad_ext.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ad_ext.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ad_ext.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ad_ext.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ad_ext.o: $(ARBHOME)/INCLUDE/awt.hxx
ad_transpro.o:  
ad_transpro.o:  
ad_transpro.o:  
ad_transpro.o:  
ad_transpro.o:  
ad_transpro.o:  
ad_transpro.o:  
ad_transpro.o:  
ad_transpro.o:  
ad_transpro.o:  
ad_transpro.o: $(ARBHOME)/INCLUDE/arbdb.h
ad_transpro.o: $(ARBHOME)/INCLUDE/ad_prot.h
ad_transpro.o: $(ARBHOME)/INCLUDE/arbdbt.h
ad_transpro.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ad_transpro.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ad_transpro.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ad_transpro.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ad_transpro.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ad_transpro.o: $(ARBHOME)/INCLUDE/awt.hxx
ad_transpro.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
ad_transpro.o: $(ARBHOME)/INCLUDE/awt_codon_table.hxx
AP_consensus.o:  
AP_consensus.o:  
AP_consensus.o:  
AP_consensus.o:  
AP_consensus.o:  
AP_consensus.o:  
AP_consensus.o:  
AP_consensus.o:  
AP_consensus.o:  
AP_consensus.o:  $(ARBHOME)/INCLUDE/arbdb.h
AP_consensus.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_consensus.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_consensus.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_consensus.o: 
AP_consensus.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AP_consensus.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AP_consensus.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AP_consensus.o: $(ARBHOME)/INCLUDE/awt.hxx
AP_cprofile.o:  
AP_cprofile.o:  
AP_cprofile.o:  
AP_cprofile.o:  
AP_cprofile.o:  
AP_cprofile.o:  
AP_cprofile.o:  
AP_cprofile.o:  
AP_cprofile.o:  
AP_cprofile.o:  
AP_cprofile.o:  
AP_cprofile.o:  
AP_cprofile.o: 
AP_cprofile.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_cprofile.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_cprofile.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_cprofile.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_cprofile.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AP_cprofile.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AP_cprofile.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AP_cprofile.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AP_cprofile.o: $(ARBHOME)/INCLUDE/awt.hxx
AP_pos_var_pars.o:  
AP_pos_var_pars.o:  
AP_pos_var_pars.o:  
AP_pos_var_pars.o:  
AP_pos_var_pars.o:  
AP_pos_var_pars.o:  
AP_pos_var_pars.o:  
AP_pos_var_pars.o:  
AP_pos_var_pars.o:  
AP_pos_var_pars.o:  
AP_pos_var_pars.o:  
AP_pos_var_pars.o:  
AP_pos_var_pars.o:  
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/awt.hxx
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
AP_pos_var_pars.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
AP_pos_var_pars.o: ap_pos_var_pars.hxx
ETC_check_gcg.o:  
ETC_check_gcg.o:  
ETC_check_gcg.o:  
ETC_check_gcg.o:  
ETC_check_gcg.o:  
ETC_check_gcg.o:  
ETC_check_gcg.o:  
ETC_check_gcg.o:  
ETC_check_gcg.o:  
ETC_check_gcg.o:  
ETC_check_gcg.o: $(ARBHOME)/INCLUDE/arbdb.h
ETC_check_gcg.o: $(ARBHOME)/INCLUDE/ad_prot.h
ETC_check_gcg.o: $(ARBHOME)/INCLUDE/arbdbt.h
ETC_check_gcg.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ETC_check_gcg.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ETC_check_gcg.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ETC_check_gcg.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ETC_check_gcg.o: $(ARBHOME)/INCLUDE/awt.hxx
AP_csp_2_gnuplot.o:  
AP_csp_2_gnuplot.o:  
AP_csp_2_gnuplot.o:  
AP_csp_2_gnuplot.o:  
AP_csp_2_gnuplot.o:  
AP_csp_2_gnuplot.o:  
AP_csp_2_gnuplot.o:  
AP_csp_2_gnuplot.o:  
AP_csp_2_gnuplot.o:  
AP_csp_2_gnuplot.o:  
AP_csp_2_gnuplot.o:  
AP_csp_2_gnuplot.o: 
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/awt.hxx
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
AP_csp_2_gnuplot.o: $(ARBHOME)/INCLUDE/awt_csp.hxx
AP_csp_2_gnuplot.o: ap_csp_2_gnuplot.hxx
