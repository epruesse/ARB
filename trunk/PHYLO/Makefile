# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

CPP_OBJECTS =  PH_data.o PH_root.o PH_main.o PH_filt.o PH_display.o PH_matr.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)

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

PH_data.o: include.hxx
PH_data.o: phwin.hxx
PH_data.o: phylo.hxx
PH_data.o: $(ARBHOME)/INCLUDE/ad_prot.h
PH_data.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PH_data.o: $(ARBHOME)/INCLUDE/arb_assert.h
PH_data.o: $(ARBHOME)/INCLUDE/arbdb.h
PH_data.o: $(ARBHOME)/INCLUDE/arbdbt.h
PH_data.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
PH_data.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PH_data.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
PH_data.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
PH_data.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PH_data.o: $(ARBHOME)/INCLUDE/aw_window.hxx
PH_data.o: $(ARBHOME)/INCLUDE/awt.hxx
PH_data.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
PH_data.o: $(ARBHOME)/INCLUDE/awt_nds.hxx
PH_data.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
PH_data.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
PH_data.o: $(ARBHOME)/INCLUDE/awt_tree.hxx

PH_display.o: include.hxx
PH_display.o: PH_display.hxx
PH_display.o: phwin.hxx
PH_display.o: phylo.hxx
PH_display.o: $(ARBHOME)/INCLUDE/ad_prot.h
PH_display.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PH_display.o: $(ARBHOME)/INCLUDE/arb_assert.h
PH_display.o: $(ARBHOME)/INCLUDE/arbdb.h
PH_display.o: $(ARBHOME)/INCLUDE/arbdbt.h
PH_display.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PH_display.o: $(ARBHOME)/INCLUDE/aw_window.hxx
PH_display.o: $(ARBHOME)/INCLUDE/awt.hxx
PH_display.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
PH_display.o: $(ARBHOME)/INCLUDE/awt_nds.hxx
PH_display.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
PH_display.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
PH_display.o: $(ARBHOME)/INCLUDE/awt_tree.hxx

PH_filt.o: include.hxx
PH_filt.o: phwin.hxx
PH_filt.o: phylo.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/ad_prot.h
PH_filt.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PH_filt.o: $(ARBHOME)/INCLUDE/arb_assert.h
PH_filt.o: $(ARBHOME)/INCLUDE/arbdb.h
PH_filt.o: $(ARBHOME)/INCLUDE/arbdbt.h
PH_filt.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/aw_window.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/awt.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/awt_nds.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
PH_filt.o: $(ARBHOME)/INCLUDE/awt_tree.hxx

PH_main.o: include.hxx
PH_main.o: PH_display.hxx
PH_main.o: phwin.hxx
PH_main.o: phylo.hxx
PH_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
PH_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PH_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
PH_main.o: $(ARBHOME)/INCLUDE/arbdb.h
PH_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
PH_main.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PH_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
PH_main.o: $(ARBHOME)/INCLUDE/awt.hxx
PH_main.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
PH_main.o: $(ARBHOME)/INCLUDE/awt_nds.hxx
PH_main.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
PH_main.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
PH_main.o: $(ARBHOME)/INCLUDE/awt_tree.hxx

PH_matr.o: include.hxx
PH_matr.o: phwin.hxx
PH_matr.o: phylo.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/ad_prot.h
PH_matr.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PH_matr.o: $(ARBHOME)/INCLUDE/arb_assert.h
PH_matr.o: $(ARBHOME)/INCLUDE/arbdb.h
PH_matr.o: $(ARBHOME)/INCLUDE/arbdbt.h
PH_matr.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/aw_global.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/aw_window.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/awt.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/awt_nds.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
PH_matr.o: $(ARBHOME)/INCLUDE/awt_tree.hxx

PH_root.o: include.hxx
PH_root.o: phwin.hxx
PH_root.o: phylo.hxx
PH_root.o: $(ARBHOME)/INCLUDE/ad_prot.h
PH_root.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PH_root.o: $(ARBHOME)/INCLUDE/arb_assert.h
PH_root.o: $(ARBHOME)/INCLUDE/arbdb.h
PH_root.o: $(ARBHOME)/INCLUDE/arbdbt.h
PH_root.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
PH_root.o: $(ARBHOME)/INCLUDE/aw_device.hxx
PH_root.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
PH_root.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
PH_root.o: $(ARBHOME)/INCLUDE/aw_root.hxx
PH_root.o: $(ARBHOME)/INCLUDE/aw_window.hxx
PH_root.o: $(ARBHOME)/INCLUDE/awt.hxx
PH_root.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
PH_root.o: $(ARBHOME)/INCLUDE/awt_nds.hxx
PH_root.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
PH_root.o: $(ARBHOME)/INCLUDE/awt_seq_dna.hxx
PH_root.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
