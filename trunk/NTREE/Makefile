# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

#                         Note: NT_cb.o should be first object (needed by 'proto')
CPP_OBJECTS = \
	NT_cb.o \
	AP_consensus.o \
	AP_conservProfile2Gnuplot.o \
	AP_cprofile.o \
	AP_csp_2_gnuplot.o \
	AP_pos_var_pars.o \
	NT_concatenate.o \
	NT_dbrepair.o \
	NT_edconf.o \
	NT_extern.o \
	NT_import.o \
	NT_ins_col.o \
	NT_join.o \
	NT_main.o \
	NT_sort.o \
	NT_trackAliChanges.o \
	NT_validManual.o \
	NT_validNameParser.o \
	NT_validNames.o \
	NT_tree_cmp.o \
	ad_ali.o \
	ad_ext.o \
	ad_spec.o \
	ad_transpro.o \
	ad_trees.o \


CPP_SOURCES = $(CPP_OBJECTS:.o=.cxx)

$(MAIN): $(CPP_OBJECTS)
	@$(ARBHOME)/SOURCE_TOOLS/binuptodate.pl $(MAIN) $(CPP_OBJECTS) || ( \
		echo "$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)"; \
		$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS) ; \
	)

proto:
	../MAKEBIN/aisc_mkpt -P -A -E -g -F NT_,AP_ -w nt_internal.h $(CPP_SOURCES) >nt_internal.h.tmp
	../SOURCE_TOOLS/mv_if_diff nt_internal.h.tmp nt_internal.h
	../MAKEBIN/aisc_mkpt -P -A -E -g -w nt_cb.hxx NT_cb.cxx NT_extern.cxx >nt_cb.hxx.tmp
	../SOURCE_TOOLS/mv_if_diff nt_cb.hxx.tmp nt_cb.hxx

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

clean:
	rm -f $(CPP_OBJECTS) *.a

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

NT_cb.o: nt_internal.h
NT_cb.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
NT_cb.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
NT_cb.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
NT_cb.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
NT_cb.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
NT_cb.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
NT_cb.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
NT_cb.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
NT_cb.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
NT_cb.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
NT_cb.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
NT_cb.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h nt_cb.hxx
NT_cb.o: ntree.hxx
NT_cb.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
NT_cb.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
NT_cb.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
NT_cb.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
NT_cb.o: ad_trees.hxx
NT_cb.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_canvas.hxx
NT_cb.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_sel_boxes.hxx
NT_cb.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_awars.hxx

AP_consensus.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
AP_consensus.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
AP_consensus.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
AP_consensus.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
AP_consensus.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
AP_consensus.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
AP_consensus.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
AP_consensus.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
AP_consensus.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
AP_consensus.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
AP_consensus.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
AP_consensus.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
AP_consensus.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
AP_consensus.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
AP_consensus.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
AP_consensus.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
AP_consensus.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt.hxx
AP_consensus.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_sel_boxes.hxx

AP_conservProfile2Gnuplot.o: nt_internal.h
AP_conservProfile2Gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
AP_conservProfile2Gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
AP_conservProfile2Gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
AP_conservProfile2Gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
AP_conservProfile2Gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
AP_conservProfile2Gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
AP_conservProfile2Gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
AP_conservProfile2Gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
AP_conservProfile2Gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
AP_conservProfile2Gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
AP_conservProfile2Gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
AP_conservProfile2Gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
AP_conservProfile2Gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt.hxx
AP_conservProfile2Gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
AP_conservProfile2Gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
AP_conservProfile2Gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
AP_conservProfile2Gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
AP_conservProfile2Gnuplot.o: ap_conservProfile2Gnuplot.hxx
AP_conservProfile2Gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_global.hxx

AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_awars.hxx
AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_global.hxx
AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt.hxx
AP_cprofile.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_sel_boxes.hxx

AP_csp_2_gnuplot.o: nt_internal.h
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
AP_csp_2_gnuplot.o: ap_csp_2_gnuplot.hxx
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_filter.hxx
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt.hxx
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_csp.hxx
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/AP_filter.hxx
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_global.hxx
AP_csp_2_gnuplot.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_awars.hxx

AP_pos_var_pars.o: ap_pos_var_pars.hxx
AP_pos_var_pars.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
AP_pos_var_pars.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
AP_pos_var_pars.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
AP_pos_var_pars.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
AP_pos_var_pars.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
AP_pos_var_pars.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
AP_pos_var_pars.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
AP_pos_var_pars.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
AP_pos_var_pars.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/AP_pro_a_nucs.hxx
AP_pos_var_pars.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_sel_boxes.hxx
AP_pos_var_pars.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
AP_pos_var_pars.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
AP_pos_var_pars.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
AP_pos_var_pars.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
AP_pos_var_pars.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
AP_pos_var_pars.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
AP_pos_var_pars.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
AP_pos_var_pars.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx

NT_concatenate.o: nt_internal.h
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt.hxx
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_item_sel_list.hxx
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_sel_boxes.hxx
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/AW_rename.hxx
NT_concatenate.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_question.hxx

NT_dbrepair.o: NT_dbrepair.hxx
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/adGene.h
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt.hxx
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/GEN.hxx
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/EXP.hxx
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_color_groups.hxx
NT_dbrepair.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/inline.h

NT_edconf.o: nt_cb.hxx
NT_edconf.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
NT_edconf.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
NT_edconf.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
NT_edconf.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
NT_edconf.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
NT_edconf.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
NT_edconf.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
NT_edconf.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
NT_edconf.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
NT_edconf.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
NT_edconf.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
NT_edconf.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
NT_edconf.o: nt_internal.h
NT_edconf.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_sel_boxes.hxx
NT_edconf.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
NT_edconf.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
NT_edconf.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
NT_edconf.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
NT_edconf.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_awars.hxx
NT_edconf.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_config.h

NT_extern.o: nt_internal.h
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
NT_extern.o: ntree.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
NT_extern.o: ad_spec.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt.hxx
NT_extern.o: ad_trees.hxx ap_consensus.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/seq_quality.h
NT_extern.o: nt_join.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/st_window.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/GEN.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/adGene.h
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/EXP.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/TreeCallbacks.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/TreeDisplay.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/AP_Tree.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ARB_Tree.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/AliView.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/AP_sequence.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_color_groups.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/downcast.h
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_canvas.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/AW_rename.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/probe_design.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/primer_design.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/gde.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awtc_submission.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awti_export.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_preset.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_macro.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_advice.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_config_manager.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_input_mask.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_sel_boxes.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_www.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_nds.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_preset.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_global.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_awars.hxx
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_version.h
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_build.h
NT_extern.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/svn_revision.h

NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awti_import.hxx
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_canvas.hxx
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/mg_merge.hxx
NT_import.o: nt_internal.h ad_spec.hxx
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt.hxx
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/GEN.hxx
NT_import.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/adGene.h

NT_ins_col.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
NT_ins_col.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
NT_ins_col.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
NT_ins_col.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
NT_ins_col.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
NT_ins_col.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
NT_ins_col.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
NT_ins_col.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
NT_ins_col.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
NT_ins_col.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
NT_ins_col.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
NT_ins_col.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
NT_ins_col.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
NT_ins_col.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
NT_ins_col.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
NT_ins_col.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
NT_ins_col.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_awars.hxx

NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_awars.hxx
NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt.hxx
NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_item_sel_list.hxx
NT_join.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_sel_boxes.hxx

NT_main.o: nt_internal.h
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
NT_main.o: ntree.hxx
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
NT_main.o: nt_cb.hxx
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/mg_merge.hxx
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awti_import.hxx
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_advice.hxx
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt.hxx
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_global.hxx
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_question.hxx
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_awars.hxx
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/adGene.h
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_version.h
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_build.h
NT_main.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/svn_revision.h

NT_trackAliChanges.o: NT_trackAliChanges.h
NT_trackAliChanges.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
NT_trackAliChanges.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
NT_trackAliChanges.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
NT_trackAliChanges.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
NT_trackAliChanges.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
NT_trackAliChanges.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
NT_trackAliChanges.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
NT_trackAliChanges.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
NT_trackAliChanges.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_sel_boxes.hxx
NT_trackAliChanges.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
NT_trackAliChanges.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
NT_trackAliChanges.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
NT_trackAliChanges.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
NT_trackAliChanges.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
NT_trackAliChanges.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
NT_trackAliChanges.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
NT_trackAliChanges.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx

NT_validManual.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
NT_validManual.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
NT_validManual.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
NT_validManual.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
NT_validManual.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
NT_validManual.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
NT_validManual.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
NT_validManual.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
NT_validManual.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
NT_validManual.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
NT_validManual.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
NT_validManual.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
NT_validManual.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
NT_validManual.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
NT_validManual.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
NT_validManual.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
NT_validManual.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_awars.hxx
NT_validManual.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt.hxx

NT_validNameParser.o: nt_validNameParser.hxx

NT_validNames.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
NT_validNames.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
NT_validNames.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
NT_validNames.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
NT_validNames.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
NT_validNames.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
NT_validNames.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
NT_validNames.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
NT_validNames.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
NT_validNames.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
NT_validNames.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
NT_validNames.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
NT_validNames.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
NT_validNames.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
NT_validNames.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
NT_validNames.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
NT_validNames.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_awars.hxx
NT_validNames.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt.hxx
NT_validNames.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_www.hxx
NT_validNames.o: nt_validNameParser.hxx

NT_tree_cmp.o: nt_tree_cmp.hxx
NT_tree_cmp.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
NT_tree_cmp.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
NT_tree_cmp.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
NT_tree_cmp.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
NT_tree_cmp.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
NT_tree_cmp.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
NT_tree_cmp.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
NT_tree_cmp.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
NT_tree_cmp.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/AP_Tree.hxx
NT_tree_cmp.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ARB_Tree.hxx
NT_tree_cmp.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/AliView.hxx
NT_tree_cmp.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/AP_sequence.hxx
NT_tree_cmp.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_color_groups.hxx
NT_tree_cmp.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
NT_tree_cmp.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
NT_tree_cmp.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
NT_tree_cmp.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
NT_tree_cmp.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/downcast.h

ad_ali.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
ad_ali.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
ad_ali.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
ad_ali.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
ad_ali.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
ad_ali.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
ad_ali.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
ad_ali.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
ad_ali.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
ad_ali.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
ad_ali.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
ad_ali.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
ad_ali.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
ad_ali.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
ad_ali.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
ad_ali.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
ad_ali.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt.hxx
ad_ali.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_sel_boxes.hxx

ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt.hxx
ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_sel_boxes.hxx
ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_awars.hxx
ad_ext.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/db_scanner.hxx

ad_spec.o: ad_spec.hxx /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt.hxx
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
ad_spec.o: nt_internal.h
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
ad_spec.o: ntree.hxx
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awtc_next_neighbours.hxx
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/AW_rename.hxx
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/db_scanner.hxx
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_item_sel_list.hxx
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awtlocal.hxx
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_www.hxx
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_canvas.hxx
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_sel_boxes.hxx
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_awars.hxx
ad_spec.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/probe_design.hxx

ad_transpro.o: nt_internal.h
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_sel_boxes.hxx
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/Translate.hxx
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/AP_codon_table.hxx
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt.hxx
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/AP_pro_a_nucs.hxx
ad_transpro.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_awars.hxx

ad_trees.o: ad_trees.hxx
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_root.hxx
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_assert.h
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb_base.h
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/attributes.h
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_k_prot.h
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbtools.h
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arb_error.h
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/smartptr.h
ad_trees.o: nt_tree_cmp.hxx
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdbt.h
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/arbdb.h
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_prot.h
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/ad_t_prot.h
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/TreeRead.h
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/TreeWrite.h
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_sel_boxes.hxx
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt_nds.hxx
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/awt.hxx
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_window.hxx
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_device.hxx
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_position.hxx
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_keysym.hxx
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_awars.hxx
ad_trees.o: /home/ralf/ARB-saphira/ARB.quickChange/INCLUDE/aw_global.hxx
