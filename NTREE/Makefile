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


AP_consensus.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_consensus.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_consensus.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_consensus.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_consensus.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AP_consensus.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AP_consensus.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AP_consensus.c.o: $(ARBHOME)/INCLUDE/awt.hxx
AP_consensus.c.o:  
AP_consensus.c.o:  
AP_consensus.c.o:  
AP_consensus.c.o:  
AP_consensus.c.o:  
AP_consensus.c.o:  
AP_consensus.c.o:  
AP_consensus.c.o:  
AP_consensus.c.o:  
AP_consensus.c.o:  
AP_consensus.c.o:  
AP_consensus.c.o: 
AP_consensus.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AP_consensus.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AP_cprofile.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_cprofile.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_cprofile.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_cprofile.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_cprofile.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AP_cprofile.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AP_cprofile.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AP_cprofile.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AP_cprofile.c.o: $(ARBHOME)/INCLUDE/awt.hxx
AP_cprofile.c.o:  
AP_cprofile.c.o:  
AP_cprofile.c.o:  
AP_cprofile.c.o:  
AP_cprofile.c.o:  
AP_cprofile.c.o:  
AP_cprofile.c.o:  
AP_cprofile.c.o:  
AP_cprofile.c.o:  
AP_cprofile.c.o:  
AP_cprofile.c.o:  
AP_cprofile.c.o:  
AP_cprofile.c.o:  
AP_cprofile.c.o:  
AP_cprofile.c.o:  
AP_cprofile.c.o: 
AP_cprofile.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
AP_cprofile.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AP_cprofile.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AP_csp_2_gnuplot.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_csp_2_gnuplot.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_csp_2_gnuplot.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_csp_2_gnuplot.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_csp_2_gnuplot.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AP_csp_2_gnuplot.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AP_csp_2_gnuplot.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AP_csp_2_gnuplot.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AP_csp_2_gnuplot.c.o: $(ARBHOME)/INCLUDE/awt.hxx
AP_csp_2_gnuplot.c.o: $(ARBHOME)/INCLUDE/awt_csp.hxx
AP_csp_2_gnuplot.c.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
AP_csp_2_gnuplot.c.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
AP_csp_2_gnuplot.c.o:  
AP_csp_2_gnuplot.c.o:  
AP_csp_2_gnuplot.c.o: 
AP_csp_2_gnuplot.c.o:  
AP_csp_2_gnuplot.c.o: 
AP_csp_2_gnuplot.c.o:  
AP_csp_2_gnuplot.c.o:  
AP_csp_2_gnuplot.c.o:  
AP_csp_2_gnuplot.c.o:  
AP_csp_2_gnuplot.c.o:  
AP_csp_2_gnuplot.c.o:  
AP_csp_2_gnuplot.c.o:  
AP_csp_2_gnuplot.c.o:  
AP_csp_2_gnuplot.c.o:  
AP_csp_2_gnuplot.c.o: 
AP_csp_2_gnuplot.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
AP_csp_2_gnuplot.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AP_csp_2_gnuplot.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AP_csp_2_gnuplot.c.o: ap_csp_2_gnuplot.hxx
AP_pos_var_pars.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
AP_pos_var_pars.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AP_pos_var_pars.c.o: $(ARBHOME)/INCLUDE/arbdb.h
AP_pos_var_pars.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
AP_pos_var_pars.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AP_pos_var_pars.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AP_pos_var_pars.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AP_pos_var_pars.c.o: $(ARBHOME)/INCLUDE/awt.hxx
AP_pos_var_pars.c.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
AP_pos_var_pars.c.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
AP_pos_var_pars.c.o:  
AP_pos_var_pars.c.o:  
AP_pos_var_pars.c.o:  
AP_pos_var_pars.c.o:  
AP_pos_var_pars.c.o:  
AP_pos_var_pars.c.o:  
AP_pos_var_pars.c.o:  
AP_pos_var_pars.c.o:  
AP_pos_var_pars.c.o:  
AP_pos_var_pars.c.o:  
AP_pos_var_pars.c.o:  
AP_pos_var_pars.c.o:  
AP_pos_var_pars.c.o:  
AP_pos_var_pars.c.o:  
AP_pos_var_pars.c.o:  
AP_pos_var_pars.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
AP_pos_var_pars.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
AP_pos_var_pars.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
AP_pos_var_pars.c.o: ap_pos_var_pars.hxx
ETC_check_gcg.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ETC_check_gcg.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ETC_check_gcg.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ETC_check_gcg.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ETC_check_gcg.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ETC_check_gcg.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ETC_check_gcg.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ETC_check_gcg.c.o: $(ARBHOME)/INCLUDE/awt.hxx
ETC_check_gcg.c.o:  
ETC_check_gcg.c.o:  
ETC_check_gcg.c.o:  
ETC_check_gcg.c.o:  
ETC_check_gcg.c.o:  
ETC_check_gcg.c.o:  
ETC_check_gcg.c.o:  
ETC_check_gcg.c.o:  
ETC_check_gcg.c.o:  
ETC_check_gcg.c.o:  
ETC_check_gcg.c.o:  
ETC_check_gcg.c.o: 
ETC_check_gcg.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ETC_check_gcg.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
NT_cb.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_cb.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_cb.c.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_cb.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_cb.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
NT_cb.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_cb.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_cb.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_cb.c.o: $(ARBHOME)/INCLUDE/awt.hxx
NT_cb.c.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
NT_cb.c.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
NT_cb.c.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
NT_cb.c.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
NT_cb.c.o:  
NT_cb.c.o:  
NT_cb.c.o:  
NT_cb.c.o:  
NT_cb.c.o:  
NT_cb.c.o:  
NT_cb.c.o:   
NT_cb.c.o:  
NT_cb.c.o:  
NT_cb.c.o:  
NT_cb.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
NT_cb.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
NT_cb.c.o: ad_trees.hxx ntree.hxx
NT_edconf.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_edconf.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_edconf.c.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_edconf.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_edconf.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
NT_edconf.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_edconf.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_edconf.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_edconf.c.o: $(ARBHOME)/INCLUDE/awt.hxx
NT_edconf.c.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
NT_edconf.c.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
NT_edconf.c.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
NT_edconf.c.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
NT_edconf.c.o:  
NT_edconf.c.o:  
NT_edconf.c.o:  
NT_edconf.c.o:  
NT_edconf.c.o:  
NT_edconf.c.o:  
NT_edconf.c.o:  
NT_edconf.c.o:  
NT_edconf.c.o:  
NT_edconf.c.o:  
NT_edconf.c.o:  
NT_edconf.c.o: 
NT_edconf.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
NT_edconf.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
NT_edconf.c.o: nt_edconf.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_extern.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_extern.c.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_extern.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_extern.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/awt.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/awt_nds.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/awt_preset.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/awt_tree_cb.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/awt_www.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/awtc_rename.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/awtc_submission.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/awti_export.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/gde.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/probe_design.hxx
NT_extern.c.o: $(ARBHOME)/INCLUDE/st_window.hxx
NT_extern.c.o:  
NT_extern.c.o:  
NT_extern.c.o:  
NT_extern.c.o:  
NT_extern.c.o:  
NT_extern.c.o:  
NT_extern.c.o:   
NT_extern.c.o:  
NT_extern.c.o:  
NT_extern.c.o:  
NT_extern.c.o: 
NT_extern.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
NT_extern.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
NT_extern.c.o: ad_ali.hxx ad_ext.hxx ad_spec.hxx ad_trees.hxx ap_consensus.hxx
NT_extern.c.o: ap_csp_2_gnuplot.hxx ap_pos_var_pars.hxx etc_check_gcg.hxx
NT_extern.c.o: nt_cb.hxx nt_date.h nt_edconf.hxx nt_import.hxx nt_join.hxx
NT_extern.c.o: nt_sort.hxx ntree.hxx
NT_import.c.o: $(ARBHOME)/INCLUDE/GEN.hxx
NT_import.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_import.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_import.c.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_import.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_import.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_import.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_import.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_import.c.o: $(ARBHOME)/INCLUDE/awti_import.hxx
NT_import.c.o: $(ARBHOME)/INCLUDE/mg_merge.hxx
NT_import.c.o:  
NT_import.c.o:  
NT_import.c.o:  
NT_import.c.o:  
NT_import.c.o:  
NT_import.c.o:  
NT_import.c.o:   
NT_import.c.o:  
NT_import.c.o:  
NT_import.c.o:  
NT_import.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
NT_import.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
NT_import.c.o: ad_spec.hxx nt_import.hxx
NT_ins_col.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_ins_col.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_ins_col.c.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_ins_col.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_ins_col.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
NT_ins_col.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_ins_col.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_ins_col.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_ins_col.c.o:  
NT_ins_col.c.o:  
NT_ins_col.c.o:  
NT_ins_col.c.o:  
NT_ins_col.c.o:  
NT_ins_col.c.o:  
NT_ins_col.c.o:  
NT_ins_col.c.o:  
NT_ins_col.c.o:  
NT_ins_col.c.o:  
NT_ins_col.c.o: 
NT_ins_col.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
NT_ins_col.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
NT_join.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_join.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_join.c.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_join.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_join.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_join.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_join.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_join.c.o: $(ARBHOME)/INCLUDE/awt.hxx
NT_join.c.o:  
NT_join.c.o:  
NT_join.c.o:  
NT_join.c.o:  
NT_join.c.o:  
NT_join.c.o:  
NT_join.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
NT_join.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
NT_main.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_main.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_main.c.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_main.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_main.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
NT_main.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_main.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_main.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_main.c.o: $(ARBHOME)/INCLUDE/awt.hxx
NT_main.c.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
NT_main.c.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
NT_main.c.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
NT_main.c.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
NT_main.c.o: $(ARBHOME)/INCLUDE/awti_import.hxx
NT_main.c.o: $(ARBHOME)/INCLUDE/mg_merge.hxx
NT_main.c.o: $(ARBHOME)/INCLUDE/seer.hxx
NT_main.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
NT_main.c.o:  
NT_main.c.o:  
NT_main.c.o:  
NT_main.c.o:  
NT_main.c.o:  
NT_main.c.o:  
NT_main.c.o:   
NT_main.c.o:  
NT_main.c.o:  
NT_main.c.o:  
NT_main.c.o:  
NT_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
NT_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
NT_main.c.o: nt_cb.hxx ntree.hxx
NT_sort.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
NT_sort.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
NT_sort.c.o: $(ARBHOME)/INCLUDE/arbdb.h
NT_sort.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
NT_sort.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
NT_sort.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
NT_sort.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
NT_sort.c.o: $(ARBHOME)/INCLUDE/awt.hxx
NT_sort.c.o:  
NT_sort.c.o:  
NT_sort.c.o:  
NT_sort.c.o:  
NT_sort.c.o:  
NT_sort.c.o:  
NT_sort.c.o:   
NT_sort.c.o:  
NT_sort.c.o:  
NT_sort.c.o:  
NT_sort.c.o:  
NT_sort.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
NT_sort.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
NT_sort.c.o: nt_sort.hxx
ad_ali.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ad_ali.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ad_ali.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ad_ali.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ad_ali.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ad_ali.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ad_ali.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ad_ali.c.o: $(ARBHOME)/INCLUDE/awt.hxx
ad_ali.c.o:  
ad_ali.c.o:  
ad_ali.c.o:  
ad_ali.c.o:  
ad_ali.c.o:  
ad_ali.c.o:  
ad_ali.c.o:   
ad_ali.c.o:   
ad_ali.c.o:  
ad_ali.c.o:  
ad_ali.c.o:  
ad_ali.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ad_ali.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ad_ext.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ad_ext.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ad_ext.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ad_ext.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ad_ext.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ad_ext.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ad_ext.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ad_ext.c.o: $(ARBHOME)/INCLUDE/awt.hxx
ad_ext.c.o:  
ad_ext.c.o:  
ad_ext.c.o:  
ad_ext.c.o:  
ad_ext.c.o:   
ad_ext.c.o:  
ad_ext.c.o: 
ad_ext.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ad_ext.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ad_spec.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ad_spec.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ad_spec.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ad_spec.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ad_spec.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ad_spec.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ad_spec.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ad_spec.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ad_spec.c.o: $(ARBHOME)/INCLUDE/awt.hxx
ad_spec.c.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
ad_spec.c.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
ad_spec.c.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
ad_spec.c.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
ad_spec.c.o: $(ARBHOME)/INCLUDE/awt_www.hxx
ad_spec.c.o: $(ARBHOME)/INCLUDE/awtc_next_neighbours.hxx
ad_spec.c.o: $(ARBHOME)/INCLUDE/awtlocal.hxx
ad_spec.c.o: $(ARBHOME)/INCLUDE/probe_design.hxx
ad_spec.c.o:  
ad_spec.c.o:  
ad_spec.c.o:  
ad_spec.c.o:  
ad_spec.c.o:  
ad_spec.c.o:  
ad_spec.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ad_spec.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ad_spec.c.o: ad_spec.hxx ntree.hxx
ad_transpro.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ad_transpro.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ad_transpro.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ad_transpro.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ad_transpro.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ad_transpro.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ad_transpro.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ad_transpro.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ad_transpro.c.o: $(ARBHOME)/INCLUDE/awt.hxx
ad_transpro.c.o: $(ARBHOME)/INCLUDE/awt_codon_table.hxx
ad_transpro.c.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
ad_transpro.c.o:  
ad_transpro.c.o:  
ad_transpro.c.o:  
ad_transpro.c.o:  
ad_transpro.c.o:  
ad_transpro.c.o:  
ad_transpro.c.o:  
ad_transpro.c.o:  
ad_transpro.c.o:  
ad_transpro.c.o:  
ad_transpro.c.o:  
ad_transpro.c.o: 
ad_transpro.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ad_transpro.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ad_trees.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ad_trees.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ad_trees.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ad_trees.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ad_trees.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ad_trees.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ad_trees.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ad_trees.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ad_trees.c.o: $(ARBHOME)/INCLUDE/awt.hxx
ad_trees.c.o: $(ARBHOME)/INCLUDE/awt_imp_exp.hxx
ad_trees.c.o: $(ARBHOME)/INCLUDE/awt_tree_cmp.hxx
ad_trees.c.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ad_trees.c.o:  
ad_trees.c.o:  
ad_trees.c.o:  
ad_trees.c.o:  
ad_trees.c.o:  
ad_trees.c.o:  
ad_trees.c.o:   
ad_trees.c.o:   
ad_trees.c.o:  
ad_trees.c.o:  
ad_trees.c.o:  
ad_trees.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ad_trees.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
