# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx

CPP_OBJECTS =   ST_quality.o ST_ml.o ST_window.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

.cxx.o: 
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)



# DO NOT DELETE THIS LINE -- make depend depends on it.

ST_quality.o:  $(ARBHOME)/INCLUDE/arbdb.h
ST_quality.o: $(ARBHOME)/INCLUDE/arb_assert.h
ST_quality.o: $(ARBHOME)/INCLUDE/ad_prot.h
ST_quality.o: $(ARBHOME)/INCLUDE/arbdbt.h
ST_quality.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ST_quality.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/awt_csp.hxx st_ml.hxx st_window.hxx
ST_quality.o: st_quality.hxx
ST_ml.o:  $(ARBHOME)/INCLUDE/arbdb.h
ST_ml.o: $(ARBHOME)/INCLUDE/arb_assert.h
ST_ml.o: $(ARBHOME)/INCLUDE/ad_prot.h
ST_ml.o: $(ARBHOME)/INCLUDE/arbdbt.h
ST_ml.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ST_ml.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/awt_csp.hxx st_ml.hxx
ST_window.o: $(ARBHOME)/INCLUDE/arbdb.h
ST_window.o: $(ARBHOME)/INCLUDE/arb_assert.h
ST_window.o: $(ARBHOME)/INCLUDE/ad_prot.h
ST_window.o: $(ARBHOME)/INCLUDE/arbdbt.h
ST_window.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ST_window.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ST_window.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ST_window.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ST_window.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ST_window.o: $(ARBHOME)/INCLUDE/awt.hxx
ST_window.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
ST_window.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
ST_window.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
ST_window.o: $(ARBHOME)/INCLUDE/awt_csp.hxx st_window.hxx st_ml.hxx
ST_window.o: st_quality.hxx
