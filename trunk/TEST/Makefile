
# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx

CPP_OBJECTS = D_main.o D_classes.o D_awt_graphic_designer.o


.cxx.o: 
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)


$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)
# DO NOT DELETE THIS LINE -- make depend depends on it.

D_main.o: $(ARBHOME)/INCLUDE/arbdb.h
D_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
D_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
D_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
D_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
D_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
D_main.o: $(ARBHOME)/INCLUDE/awt.hxx
D_main.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
D_main.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
D_main.o: $(ARBHOME)/INCLUDE/awt_preset.hxx d_classes.hxx
D_main.o: d_main.hxx d_awt_graphic_designer.hxx
D_classes.o:  d_classes.hxx
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/arbdb.h
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/arb_assert.h
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/ad_prot.h
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/aw_root.hxx
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/aw_device.hxx
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/aw_window.hxx
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/awt.hxx
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/awt_preset.hxx
D_awt_graphic_designer.o: d_awt_graphic_designer.hxx
