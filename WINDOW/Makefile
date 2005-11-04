.SUFFIXES: .o .c .cxx .depend

STD_OBJECTS =	AW_xkey.o AW_xfont.o AW_nawar.o AW_button.o \
		AW_at.o AW_print.o AW_window.o \
		AW_device.o AW_Xm.o AW_click.o AW_size.o \
		AW_xfig.o AW_xfigfont.o AW_status.o AW_preset.o \
		AW_question.o AW_global_awars.o AW_font_group.o

OPENGL_OBJECTS = GLwMDrawA.o GLwDrawA.o

ifeq ($(OPENGL),1)
OBJECTS=$(STD_OBJECTS) $(OPENGL_OBJECTS)
else
OBJECTS=$(STD_OBJECTS)
endif

$(MAIN): $(OBJECTS)
	$(ARCPPLIB) $(@:.a=).$(SHARED_LIB_SUFFIX) $(OBJECTS)
	touch $@
.cxx.o:
	$(CCPLIB) $(cflags) -c $< $(CPPINCLUDES)

.c.o:
	$(ACC) $(cflags) -c $< $(CPPINCLUDES)

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
AW_at.o: aw_root.hxx
AW_at.o: aw_window.hxx
AW_at.o: aw_xfig.hxx
AW_at.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_at.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_at.o: $(ARBHOME)/INCLUDE/arbdb.h

AW_button.o: aw_at.hxx
AW_button.o: aw_awar.hxx
AW_button.o: aw_click.hxx
AW_button.o: aw_device.hxx
AW_button.o: aw_keysym.hxx
AW_button.o: aw_print.hxx
AW_button.o: aw_root.hxx
AW_button.o: aw_size.hxx
AW_button.o: aw_window.hxx
AW_button.o: aw_window_Xm.hxx
AW_button.o: aw_Xm.hxx
AW_button.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_button.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AW_button.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_button.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_button.o: $(ARBHOME)/INCLUDE/arbdbt.h
AW_button.o: $(ARBHOME)/INCLUDE/inline.h

AW_click.o: aw_click.hxx
AW_click.o: aw_commn.hxx
AW_click.o: aw_device.hxx
AW_click.o: aw_root.hxx
AW_click.o: $(ARBHOME)/INCLUDE/arb_assert.h

AW_device.o: aw_commn.hxx
AW_device.o: aw_device.hxx
AW_device.o: aw_keysym.hxx
AW_device.o: aw_root.hxx
AW_device.o: aw_window.hxx
AW_device.o: $(ARBHOME)/INCLUDE/arb_assert.h

AW_font_group.o: aw_device.hxx
AW_font_group.o: aw_font_group.hxx
AW_font_group.o: aw_root.hxx
AW_font_group.o: $(ARBHOME)/INCLUDE/arb_assert.h

AW_global_awars.o: aw_global_awars.hxx
AW_global_awars.o: aw_root.hxx
AW_global_awars.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_global_awars.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_global_awars.o: $(ARBHOME)/INCLUDE/arbdb.h

AW_nawar.o: aw_device.hxx
AW_nawar.o: aw_keysym.hxx
AW_nawar.o: aw_nawar.hxx
AW_nawar.o: aw_root.hxx
AW_nawar.o: aw_window.hxx
AW_nawar.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_nawar.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_nawar.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_nawar.o: $(ARBHOME)/INCLUDE/awt.hxx

AW_preset.o: aw_color_groups.hxx
AW_preset.o: aw_def.hxx
AW_preset.o: aw_device.hxx
AW_preset.o: aw_keysym.hxx
AW_preset.o: aw_preset.hxx
AW_preset.o: aw_root.hxx
AW_preset.o: aw_window.hxx
AW_preset.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_preset.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_preset.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_preset.o: $(ARBHOME)/INCLUDE/awt.hxx
AW_preset.o: $(ARBHOME)/INCLUDE/awt_advice.hxx
AW_preset.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx

AW_print.o: aw_commn.hxx
AW_print.o: aw_device.hxx
AW_print.o: aw_print.hxx
AW_print.o: aw_root.hxx
AW_print.o: $(ARBHOME)/INCLUDE/arb_assert.h

AW_question.o: aw_question.hxx
AW_question.o: aw_root.hxx
AW_question.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_question.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_question.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_question.o: $(ARBHOME)/INCLUDE/arbtools.h

AW_size.o: aw_commn.hxx
AW_size.o: aw_device.hxx
AW_size.o: aw_root.hxx
AW_size.o: aw_size.hxx
AW_size.o: $(ARBHOME)/INCLUDE/arb_assert.h

AW_status.o: aw_awars.hxx
AW_status.o: aw_device.hxx
AW_status.o: aw_keysym.hxx
AW_status.o: aw_root.hxx
AW_status.o: aw_window.hxx
AW_status.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_status.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_status.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_status.o: $(ARBHOME)/INCLUDE/awt_www.hxx
AW_status.o: $(ARBHOME)/INCLUDE/SIG_PF.h

AW_window.o: aw_at.hxx
AW_window.o: aw_awar.hxx
AW_window.o: aw_click.hxx
AW_window.o: aw_commn.hxx
AW_window.o: aw_device.hxx
AW_window.o: aw_global.hxx
AW_window.o: aw_keysym.hxx
AW_window.o: aw_print.hxx
AW_window.o: aw_root.hxx
AW_window.o: aw_size.hxx
AW_window.o: aw_window.hxx
AW_window.o: aw_window_Xm.hxx
AW_window.o: aw_xfig.hxx
AW_window.o: aw_xfigfont.hxx
AW_window.o: aw_xkey.hxx
AW_window.o: aw_Xm.hxx
AW_window.o: GLwDrawA.h
AW_window.o: GLwMDrawA.h
AW_window.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_window.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AW_window.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_window.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_window.o: $(ARBHOME)/INCLUDE/arbdbt.h

AW_xfig.o: aw_device.hxx
AW_xfig.o: aw_root.hxx
AW_xfig.o: aw_xfig.hxx
AW_xfig.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_xfig.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_xfig.o: $(ARBHOME)/INCLUDE/arbdb.h

AW_xfigfont.o: aw_commn.hxx
AW_xfigfont.o: aw_device.hxx
AW_xfigfont.o: aw_keysym.hxx
AW_xfigfont.o: aw_root.hxx
AW_xfigfont.o: aw_window.hxx
AW_xfigfont.o: aw_xfig.hxx
AW_xfigfont.o: aw_xfigfont.hxx
AW_xfigfont.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_xfigfont.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_xfigfont.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_xfigfont.o: $(ARBHOME)/INCLUDE/awt.hxx

AW_xfont.o: aw_commn.hxx
AW_xfont.o: aw_def.hxx
AW_xfont.o: aw_device.hxx
AW_xfont.o: aw_root.hxx
AW_xfont.o: aw_xfont.hxx
AW_xfont.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_xfont.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_xfont.o: $(ARBHOME)/INCLUDE/arbdb.h

AW_xkey.o: aw_keysym.hxx
AW_xkey.o: aw_xkey.hxx
AW_xkey.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_xkey.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_xkey.o: $(ARBHOME)/INCLUDE/arbdb.h

AW_Xm.o: aw_commn.hxx
AW_Xm.o: aw_device.hxx
AW_Xm.o: aw_root.hxx
AW_Xm.o: aw_Xm.hxx
AW_Xm.o: $(ARBHOME)/INCLUDE/arb_assert.h
