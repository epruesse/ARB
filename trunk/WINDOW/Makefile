.SUFFIXES: .o .c .cxx .depend

OBJECTS =	AW_xkey.o AW_xfont.o AW_nawar.o AW_button.o \
		AW_at.o AW_print.o AW_window.o \
		AW_device.o AW_Xm.o AW_click.o AW_size.o \
		AW_xfig.o AW_xfigfont.o AW_status.o AW_preset.o \
		AW_question.o AW_global_awars.o

$(MAIN): $(OBJECTS)
	$(ARCPPLIB) $(@:.a=).$(SHARED_LIB_SUFFIX) $(OBJECTS)
	touch $@
.cxx.o:
	$(CCPLIB) $(cflags) -c $< $(CPPINCLUDES)

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

AW_xkey.o: aw_keysym.hxx aw_xkey.hxx $(ARBHOME)/INCLUDE/arbdb.h
AW_xkey.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_xkey.o: $(ARBHOME)/INCLUDE/ad_prot.h

AW_xfont.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_xfont.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_xfont.o: $(ARBHOME)/INCLUDE/ad_prot.h aw_root.hxx aw_device.hxx
AW_xfont.o: aw_commn.hxx aw_xfont.hxx

AW_nawar.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_nawar.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_nawar.o: $(ARBHOME)/INCLUDE/ad_prot.h aw_root.hxx aw_nawar.hxx

AW_button.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_button.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_button.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_button.o: $(ARBHOME)/INCLUDE/arbdbt.h
AW_button.o: $(ARBHOME)/INCLUDE/ad_t_prot.h aw_root.hxx aw_at.hxx
AW_button.o: aw_device.hxx aw_Xm.hxx aw_click.hxx aw_print.hxx aw_size.hxx
AW_button.o: aw_window.hxx aw_awar.hxx aw_window_Xm.hxx

AW_at.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_at.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_at.o: $(ARBHOME)/INCLUDE/ad_prot.h aw_root.hxx aw_device.hxx
AW_at.o: aw_at.hxx aw_window.hxx aw_xfig.hxx

AW_print.o: aw_root.hxx $(ARBHOME)/INCLUDE/arb_assert.h aw_device.hxx
AW_print.o: aw_commn.hxx aw_print.hxx

AW_window.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_window.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_window.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_window.o: $(ARBHOME)/INCLUDE/arbdbt.h
AW_window.o: $(ARBHOME)/INCLUDE/ad_t_prot.h aw_root.hxx aw_device.hxx
AW_window.o: aw_commn.hxx aw_keysym.hxx aw_at.hxx aw_window.hxx aw_awar.hxx
AW_window.o: aw_xfig.hxx aw_xfigfont.hxx aw_Xm.hxx aw_click.hxx aw_size.hxx
AW_window.o: aw_print.hxx aw_window_Xm.hxx aw_xkey.hxx

AW_device.o: aw_root.hxx $(ARBHOME)/INCLUDE/arb_assert.h aw_device.hxx
AW_device.o: aw_window.hxx aw_commn.hxx

AW_Xm.o: aw_root.hxx $(ARBHOME)/INCLUDE/arb_assert.h aw_device.hxx
AW_Xm.o: aw_commn.hxx aw_Xm.hxx

AW_click.o: aw_root.hxx $(ARBHOME)/INCLUDE/arb_assert.h
AW_click.o: aw_device.hxx aw_commn.hxx aw_click.hxx

AW_size.o: aw_root.hxx $(ARBHOME)/INCLUDE/arb_assert.h aw_device.hxx
AW_size.o: aw_commn.hxx aw_size.hxx

AW_xfig.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_xfig.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_xfig.o: $(ARBHOME)/INCLUDE/ad_prot.h aw_root.hxx aw_device.hxx
AW_xfig.o: aw_xfig.hxx

AW_xfigfont.o: aw_root.hxx $(ARBHOME)/INCLUDE/arb_assert.h
AW_xfigfont.o: aw_device.hxx aw_commn.hxx aw_xfig.hxx aw_xfigfont.hxx
AW_xfigfont.o: aw_window.hxx $(ARBHOME)/INCLUDE/arbdb.h
AW_xfigfont.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_xfigfont.o: $(ARBHOME)/INCLUDE/awt.hxx

AW_status.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_status.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_status.o: $(ARBHOME)/INCLUDE/ad_prot.h aw_root.hxx
AW_status.o: aw_device.hxx aw_window.hxx
AW_status.o: $(ARBHOME)/INCLUDE/awt_www.hxx
AW_status.o: $(ARBHOME)/INCLUDE/SIG_PF.h ../INCLUDE/arb_assert.h

AW_preset.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_preset.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_preset.o: $(ARBHOME)/INCLUDE/ad_prot.h aw_root.hxx
AW_preset.o: aw_device.hxx aw_window.hxx aw_color_groups.hxx
AW_preset.o: $(ARBHOME)/INCLUDE/awt.hxx
AW_preset.o: $(ARBHOME)/INCLUDE/awt_advice.hxx
AW_preset.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx aw_preset.hxx

AW_question.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_question.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_question.o: $(ARBHOME)/INCLUDE/ad_prot.h aw_question.hxx
AW_question.o: aw_root.hxx $(ARBHOME)/INCLUDE/arbtools.h

AW_global_awars.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_global_awars.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_global_awars.o: $(ARBHOME)/INCLUDE/ad_prot.h aw_root.hxx
AW_global_awars.o: aw_global_awars.hxx
