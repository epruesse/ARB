# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx
CPP_OBJECTS = TB_main.o TB_extern.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

.cxx.o: 
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)
# DO NOT DELETE THIS LINE -- make depend depends on it.

TB_main.o:
TB_main.o:
TB_main.o: $(ARBHOME)/INCLUDE/arbdb.h
TB_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
TB_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
TB_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
TB_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
TB_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
TB_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
TB_main.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
TB_main.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
TB_main.o: $(ARBHOME)/INCLUDE/awt_nds.hxx
TB_main.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
TB_main.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
TB_main.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx tb_extern.hxx
TB_extern.o:
TB_extern.o:
TB_extern.o: $(ARBHOME)/INCLUDE/arbdb.h
TB_extern.o: $(ARBHOME)/INCLUDE/ad_prot.h
TB_extern.o: $(ARBHOME)/INCLUDE/arbdbt.h
TB_extern.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
TB_extern.o: $(ARBHOME)/INCLUDE/aw_root.hxx
TB_extern.o: $(ARBHOME)/INCLUDE/aw_window.hxx
TB_extern.o: $(ARBHOME)/INCLUDE/aw_device.hxx
TB_extern.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
TB_extern.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
TB_extern.o: $(ARBHOME)/INCLUDE/awt_preset.hxx
TB_extern.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
TB_extern.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
TB_extern.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
TB_extern.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
TB_extern.o: $(ARBHOME)/INCLUDE/awt.hxx
TB_extern.o: $(ARBHOME)/INCLUDE/awt_tree_cb.hxx tb_extern.hxx
