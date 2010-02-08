.SUFFIXES: .o .c .cxx .depend

OBJECTS = \
		AW_position.o \
		AW_edit.o \
		AW_Xm.o \
		AW_at.o \
		AW_button.o \
		AW_click.o \
		AW_debug.o \
		AW_device.o \
		AW_font_group.o \
		AW_global_awars.o \
		AW_nawar.o \
		AW_preset.o \
		AW_print.o \
		AW_question.o \
		AW_size.o \
		AW_status.o \
		AW_window.o \
		AW_xfig.o \
		AW_xfigfont.o \
		AW_xfont.o \
		AW_xkey.o \
		AW_window_Xm_interface.o \


$(MAIN): $(OBJECTS)
	$(LINK_SHARED_LIB) $(@:.a=).$(SHARED_LIB_SUFFIX) $(OBJECTS)
	touch $@
.cxx.o:
	$(CPPLIB) $(cflags) -c $< $(CPPINCLUDES)

.c.o:
	$(ACCLIB) $(cflags) -c $< $(CPPINCLUDES)

clean:
	rm -f $(OBJECTS) *.a *.so

DEPENDS = $(OBJECTS:.o=.depend)
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

AW_at.o: aw_at.hxx
AW_at.o: aw_device.hxx
AW_at.o: aw_keysym.hxx
AW_at.o: aw_position.hxx
AW_at.o: aw_root.hxx
AW_at.o: aw_window.hxx
AW_at.o: aw_xfig.hxx
AW_at.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_at.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_at.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_at.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_at.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_at.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_at.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_at.o: $(ARBHOME)/INCLUDE/attributes.h
AW_at.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_at.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_button.o: aw_at.hxx
AW_button.o: aw_awar.hxx
AW_button.o: aw_click.hxx
AW_button.o: aw_device.hxx
AW_button.o: aw_keysym.hxx
AW_button.o: aw_position.hxx
AW_button.o: aw_print.hxx
AW_button.o: aw_root.hxx
AW_button.o: aw_size.hxx
AW_button.o: aw_window.hxx
AW_button.o: aw_window_Xm.hxx
AW_button.o: aw_Xm.hxx
AW_button.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_button.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_button.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AW_button.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_button.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_button.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_button.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_button.o: $(ARBHOME)/INCLUDE/arbdbt.h
AW_button.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_button.o: $(ARBHOME)/INCLUDE/attributes.h
AW_button.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_button.o: $(ARBHOME)/INCLUDE/inline.h
AW_button.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_click.o: aw_click.hxx
AW_click.o: aw_commn.hxx
AW_click.o: aw_device.hxx
AW_click.o: aw_position.hxx
AW_click.o: aw_root.hxx
AW_click.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_click.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_click.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_click.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_click.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_click.o: $(ARBHOME)/INCLUDE/attributes.h
AW_click.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_click.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_debug.o: aw_device.hxx
AW_debug.o: aw_keysym.hxx
AW_debug.o: aw_position.hxx
AW_debug.o: aw_root.hxx
AW_debug.o: aw_window.hxx
AW_debug.o: aw_window_Xm.hxx
AW_debug.o: aw_Xm.hxx
AW_debug.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_debug.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_debug.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AW_debug.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_debug.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_debug.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_debug.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_debug.o: $(ARBHOME)/INCLUDE/arbdbt.h
AW_debug.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_debug.o: $(ARBHOME)/INCLUDE/attributes.h
AW_debug.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_debug.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_device.o: aw_commn.hxx
AW_device.o: aw_device.hxx
AW_device.o: aw_keysym.hxx
AW_device.o: aw_position.hxx
AW_device.o: aw_root.hxx
AW_device.o: aw_window.hxx
AW_device.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_device.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_device.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_device.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_device.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_device.o: $(ARBHOME)/INCLUDE/attributes.h
AW_device.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_device.o: $(ARBHOME)/INCLUDE/inline.h
AW_device.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_edit.o: aw_device.hxx
AW_edit.o: aw_edit.hxx
AW_edit.o: aw_keysym.hxx
AW_edit.o: aw_position.hxx
AW_edit.o: aw_root.hxx
AW_edit.o: aw_window.hxx
AW_edit.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_edit.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_edit.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AW_edit.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_edit.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_edit.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_edit.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_edit.o: $(ARBHOME)/INCLUDE/arbdbt.h
AW_edit.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_edit.o: $(ARBHOME)/INCLUDE/attributes.h
AW_edit.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_edit.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_font_group.o: aw_device.hxx
AW_font_group.o: aw_font_group.hxx
AW_font_group.o: aw_position.hxx
AW_font_group.o: aw_root.hxx
AW_font_group.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_font_group.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_font_group.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_font_group.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_font_group.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_font_group.o: $(ARBHOME)/INCLUDE/attributes.h
AW_font_group.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_font_group.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_global_awars.o: aw_awars.hxx
AW_global_awars.o: aw_device.hxx
AW_global_awars.o: aw_global_awars.hxx
AW_global_awars.o: aw_keysym.hxx
AW_global_awars.o: aw_position.hxx
AW_global_awars.o: aw_root.hxx
AW_global_awars.o: aw_window.hxx
AW_global_awars.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_global_awars.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_global_awars.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_global_awars.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_global_awars.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_global_awars.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_global_awars.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_global_awars.o: $(ARBHOME)/INCLUDE/attributes.h
AW_global_awars.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_global_awars.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_nawar.o: aw_device.hxx
AW_nawar.o: aw_keysym.hxx
AW_nawar.o: aw_nawar.hxx
AW_nawar.o: aw_position.hxx
AW_nawar.o: aw_root.hxx
AW_nawar.o: aw_window.hxx
AW_nawar.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_nawar.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_nawar.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_nawar.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_nawar.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_nawar.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_nawar.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_nawar.o: $(ARBHOME)/INCLUDE/attributes.h
AW_nawar.o: $(ARBHOME)/INCLUDE/awt.hxx
AW_nawar.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_nawar.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_position.o: aw_position.hxx
AW_position.o: aw_root.hxx
AW_position.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_position.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_position.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_position.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_position.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_position.o: $(ARBHOME)/INCLUDE/attributes.h
AW_position.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_position.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_preset.o: aw_color_groups.hxx
AW_preset.o: aw_def.hxx
AW_preset.o: aw_device.hxx
AW_preset.o: aw_keysym.hxx
AW_preset.o: aw_position.hxx
AW_preset.o: aw_preset.hxx
AW_preset.o: aw_root.hxx
AW_preset.o: aw_window.hxx
AW_preset.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_preset.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_preset.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AW_preset.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_preset.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_preset.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_preset.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_preset.o: $(ARBHOME)/INCLUDE/arbdbt.h
AW_preset.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_preset.o: $(ARBHOME)/INCLUDE/attributes.h
AW_preset.o: $(ARBHOME)/INCLUDE/awt.hxx
AW_preset.o: $(ARBHOME)/INCLUDE/awt_advice.hxx
AW_preset.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
AW_preset.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_preset.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_print.o: aw_commn.hxx
AW_print.o: aw_device.hxx
AW_print.o: aw_position.hxx
AW_print.o: aw_print.hxx
AW_print.o: aw_root.hxx
AW_print.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_print.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_print.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_print.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_print.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_print.o: $(ARBHOME)/INCLUDE/attributes.h
AW_print.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_print.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_question.o: aw_question.hxx
AW_question.o: aw_root.hxx
AW_question.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_question.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_question.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_question.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_question.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_question.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_question.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_question.o: $(ARBHOME)/INCLUDE/attributes.h
AW_question.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_question.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_size.o: aw_commn.hxx
AW_size.o: aw_device.hxx
AW_size.o: aw_position.hxx
AW_size.o: aw_root.hxx
AW_size.o: aw_size.hxx
AW_size.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_size.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_size.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_size.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_size.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_size.o: $(ARBHOME)/INCLUDE/attributes.h
AW_size.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_size.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_status.o: aw_awars.hxx
AW_status.o: aw_device.hxx
AW_status.o: aw_edit.hxx
AW_status.o: aw_global.hxx
AW_status.o: aw_keysym.hxx
AW_status.o: aw_position.hxx
AW_status.o: aw_root.hxx
AW_status.o: aw_window.hxx
AW_status.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_status.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_status.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AW_status.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_status.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_status.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_status.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_status.o: $(ARBHOME)/INCLUDE/arbdbt.h
AW_status.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_status.o: $(ARBHOME)/INCLUDE/attributes.h
AW_status.o: $(ARBHOME)/INCLUDE/awt.hxx
AW_status.o: $(ARBHOME)/INCLUDE/awt_www.hxx
AW_status.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_status.o: $(ARBHOME)/INCLUDE/SigHandler.h
AW_status.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_window.o: aw_at.hxx
AW_window.o: aw_awar.hxx
AW_window.o: aw_click.hxx
AW_window.o: aw_commn.hxx
AW_window.o: aw_device.hxx
AW_window.o: aw_global.hxx
AW_window.o: aw_keysym.hxx
AW_window.o: aw_position.hxx
AW_window.o: aw_print.hxx
AW_window.o: aw_root.hxx
AW_window.o: aw_size.hxx
AW_window.o: aw_window.hxx
AW_window.o: aw_window_Xm.hxx
AW_window.o: aw_xfig.hxx
AW_window.o: aw_xfigfont.hxx
AW_window.o: aw_xkey.hxx
AW_window.o: aw_Xm.hxx
AW_window.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_window.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_window.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AW_window.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_window.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_window.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_window.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_window.o: $(ARBHOME)/INCLUDE/arbdbt.h
AW_window.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_window.o: $(ARBHOME)/INCLUDE/attributes.h
AW_window.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_window.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_window_Xm_interface.o: aw_commn.hxx
AW_window_Xm_interface.o: aw_device.hxx
AW_window_Xm_interface.o: aw_keysym.hxx
AW_window_Xm_interface.o: aw_position.hxx
AW_window_Xm_interface.o: aw_root.hxx
AW_window_Xm_interface.o: aw_window.hxx
AW_window_Xm_interface.o: aw_window_Xm.hxx
AW_window_Xm_interface.o: aw_window_Xm_interface.hxx
AW_window_Xm_interface.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_window_Xm_interface.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_window_Xm_interface.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_window_Xm_interface.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_window_Xm_interface.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_window_Xm_interface.o: $(ARBHOME)/INCLUDE/attributes.h
AW_window_Xm_interface.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_window_Xm_interface.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_xfig.o: aw_device.hxx
AW_xfig.o: aw_position.hxx
AW_xfig.o: aw_root.hxx
AW_xfig.o: aw_xfig.hxx
AW_xfig.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_xfig.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_xfig.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_xfig.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_xfig.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_xfig.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_xfig.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_xfig.o: $(ARBHOME)/INCLUDE/attributes.h
AW_xfig.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_xfig.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_xfigfont.o: aw_commn.hxx
AW_xfigfont.o: aw_device.hxx
AW_xfigfont.o: aw_keysym.hxx
AW_xfigfont.o: aw_position.hxx
AW_xfigfont.o: aw_root.hxx
AW_xfigfont.o: aw_window.hxx
AW_xfigfont.o: aw_xfig.hxx
AW_xfigfont.o: aw_xfigfont.hxx
AW_xfigfont.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_xfigfont.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_xfigfont.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_xfigfont.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_xfigfont.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_xfigfont.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_xfigfont.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_xfigfont.o: $(ARBHOME)/INCLUDE/attributes.h
AW_xfigfont.o: $(ARBHOME)/INCLUDE/awt.hxx
AW_xfigfont.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_xfigfont.o: $(ARBHOME)/INCLUDE/inline.h
AW_xfigfont.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_xfont.o: aw_commn.hxx
AW_xfont.o: aw_def.hxx
AW_xfont.o: aw_device.hxx
AW_xfont.o: aw_position.hxx
AW_xfont.o: aw_root.hxx
AW_xfont.o: aw_xfont.hxx
AW_xfont.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_xfont.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_xfont.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_xfont.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_xfont.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_xfont.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_xfont.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_xfont.o: $(ARBHOME)/INCLUDE/attributes.h
AW_xfont.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_xfont.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_xkey.o: aw_keysym.hxx
AW_xkey.o: aw_root.hxx
AW_xkey.o: aw_xkey.hxx
AW_xkey.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_xkey.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_xkey.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AW_xkey.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_xkey.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_xkey.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_xkey.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_xkey.o: $(ARBHOME)/INCLUDE/arbdbt.h
AW_xkey.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_xkey.o: $(ARBHOME)/INCLUDE/attributes.h
AW_xkey.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_xkey.o: $(ARBHOME)/INCLUDE/smartptr.h

AW_Xm.o: aw_commn.hxx
AW_Xm.o: aw_device.hxx
AW_Xm.o: aw_position.hxx
AW_Xm.o: aw_root.hxx
AW_Xm.o: aw_Xm.hxx
AW_Xm.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_Xm.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_Xm.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_Xm.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_Xm.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_Xm.o: $(ARBHOME)/INCLUDE/attributes.h
AW_Xm.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_Xm.o: $(ARBHOME)/INCLUDE/smartptr.h
