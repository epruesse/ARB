# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend


CPP_OBJECTS = SEC_graphic.o SEC_main.o SEC_io.o SEC_read.o SEC_split.o SEC_paint.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

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
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

SEC_graphic.o: sec_graphic.hxx
SEC_graphic.o: secedit.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEC_graphic.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
SEC_graphic.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEC_graphic.o: $(ARBHOME)/INCLUDE/arbdb.h
SEC_graphic.o: $(ARBHOME)/INCLUDE/arbdbt.h
SEC_graphic.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/aw_font_group.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/aw_global.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/aw_window.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/awt.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/BI_helix.hxx

SEC_io.o: secedit.hxx
SEC_io.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEC_io.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEC_io.o: $(ARBHOME)/INCLUDE/arbdb.h
SEC_io.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEC_io.o: $(ARBHOME)/INCLUDE/aw_font_group.hxx
SEC_io.o: $(ARBHOME)/INCLUDE/aw_root.hxx

SEC_main.o: ../EDIT4/ed4_class.hxx
SEC_main.o: ../EDIT4/ed4_defs.hxx
SEC_main.o: ../EDIT4/ed4_defs.hxx
SEC_main.o: ../EDIT4/ed4_search.hxx
SEC_main.o: sec_graphic.hxx
SEC_main.o: secedit.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEC_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
SEC_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEC_main.o: $(ARBHOME)/INCLUDE/arbdb.h
SEC_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
SEC_main.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/aw_font_group.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/awt.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/BI_helix.hxx

SEC_paint.o: ../EDIT4/ed4_class.hxx
SEC_paint.o: ../EDIT4/ed4_defs.hxx
SEC_paint.o: ../EDIT4/ed4_defs.hxx
SEC_paint.o: ../EDIT4/ed4_search.hxx
SEC_paint.o: ../EDIT4/ed4_visualizeSAI.hxx
SEC_paint.o: sec_graphic.hxx
SEC_paint.o: secedit.hxx
SEC_paint.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEC_paint.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEC_paint.o: $(ARBHOME)/INCLUDE/arbdb.h
SEC_paint.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
SEC_paint.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEC_paint.o: $(ARBHOME)/INCLUDE/aw_font_group.hxx
SEC_paint.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
SEC_paint.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SEC_paint.o: $(ARBHOME)/INCLUDE/aw_window.hxx
SEC_paint.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
SEC_paint.o: $(ARBHOME)/INCLUDE/BI_helix.hxx

SEC_read.o: secedit.hxx
SEC_read.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEC_read.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEC_read.o: $(ARBHOME)/INCLUDE/arbdb.h
SEC_read.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEC_read.o: $(ARBHOME)/INCLUDE/aw_font_group.hxx
SEC_read.o: $(ARBHOME)/INCLUDE/aw_root.hxx

SEC_split.o: secedit.hxx
SEC_split.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEC_split.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEC_split.o: $(ARBHOME)/INCLUDE/arbdb.h
SEC_split.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEC_split.o: $(ARBHOME)/INCLUDE/aw_font_group.hxx
SEC_split.o: $(ARBHOME)/INCLUDE/aw_root.hxx
