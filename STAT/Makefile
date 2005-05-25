# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

CPP_OBJECTS =   ST_quality.o ST_ml.o ST_window.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

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

ST_ml.o: st_ml.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/ad_prot.h
ST_ml.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ST_ml.o: $(ARBHOME)/INCLUDE/arb_assert.h
ST_ml.o: $(ARBHOME)/INCLUDE/arbdb.h
ST_ml.o: $(ARBHOME)/INCLUDE/arbdbt.h
ST_ml.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/awt_csp.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/awt_tree.hxx

ST_quality.o: st_ml.hxx
ST_quality.o: st_quality.hxx
ST_quality.o: st_window.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/ad_prot.h
ST_quality.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ST_quality.o: $(ARBHOME)/INCLUDE/arb_assert.h
ST_quality.o: $(ARBHOME)/INCLUDE/arbdb.h
ST_quality.o: $(ARBHOME)/INCLUDE/arbdbt.h
ST_quality.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/awt_csp.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/BI_helix.hxx

ST_window.o: st_ml.hxx
ST_window.o: st_quality.hxx
ST_window.o: st_window.hxx
ST_window.o: $(ARBHOME)/INCLUDE/ad_prot.h
ST_window.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ST_window.o: $(ARBHOME)/INCLUDE/arb_assert.h
ST_window.o: $(ARBHOME)/INCLUDE/arbdb.h
ST_window.o: $(ARBHOME)/INCLUDE/arbdbt.h
ST_window.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ST_window.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
ST_window.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ST_window.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
ST_window.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ST_window.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ST_window.o: $(ARBHOME)/INCLUDE/awt.hxx
ST_window.o: $(ARBHOME)/INCLUDE/awt_changekey.hxx
ST_window.o: $(ARBHOME)/INCLUDE/awt_csp.hxx
ST_window.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
ST_window.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
ST_window.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
