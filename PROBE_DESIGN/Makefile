# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .depend
CPP_OBJECTS = probe_design.o SaiProbeVisualization.o
$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)
.cxx.o:
	$(CPP) $(cflags) -c -o $@ $(@:.o=.cxx)  $(CPPINCLUDES)

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

probe_design.o: probe_design.hxx
probe_design.o: SaiProbeVisualization.hxx
probe_design.o: $(ARBHOME)/INCLUDE/ad_prot.h
probe_design.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
probe_design.o: $(ARBHOME)/INCLUDE/adGene.h
probe_design.o: $(ARBHOME)/INCLUDE/arb_assert.h
probe_design.o: $(ARBHOME)/INCLUDE/arbdb.h
probe_design.o: $(ARBHOME)/INCLUDE/arbdbt.h
probe_design.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_device.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_root.hxx
probe_design.o: $(ARBHOME)/INCLUDE/aw_window.hxx
probe_design.o: $(ARBHOME)/INCLUDE/awt.hxx
probe_design.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
probe_design.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
probe_design.o: $(ARBHOME)/INCLUDE/awt_iupac.hxx
probe_design.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
probe_design.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
probe_design.o: $(ARBHOME)/INCLUDE/awt_tree_cb.hxx
probe_design.o: $(ARBHOME)/INCLUDE/client.h
probe_design.o: $(ARBHOME)/INCLUDE/GEN.hxx
probe_design.o: $(ARBHOME)/INCLUDE/PT_com.h
probe_design.o: $(ARBHOME)/INCLUDE/servercntrl.h

SaiProbeVisualization.o: SaiProbeVisualization.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/ad_prot.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/arb_assert.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/arbdb.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/arbdbt.h
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/aw_window.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/awt.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
SaiProbeVisualization.o: $(ARBHOME)/INCLUDE/awt_tree_cb.hxx
