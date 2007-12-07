# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .depend

CPP_OBJECTS = D_main.o D_classes.o D_awt_graphic_designer.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)


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

D_awt_graphic_designer.o: d_awt_graphic_designer.hxx
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/ad_prot.h
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/arb_assert.h
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/arbdb.h
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/aw_device.hxx
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/aw_position.hxx
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/aw_root.hxx
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/aw_window.hxx
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/awt.hxx
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
D_awt_graphic_designer.o: $(ARBHOME)/INCLUDE/awt_preset.hxx

D_classes.o: d_classes.hxx

D_main.o: d_awt_graphic_designer.hxx
D_main.o: d_classes.hxx
D_main.o: d_main.hxx
D_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
D_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
D_main.o: $(ARBHOME)/INCLUDE/arbdb.h
D_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
D_main.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
D_main.o: $(ARBHOME)/INCLUDE/aw_position.hxx
D_main.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
D_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
D_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
D_main.o: $(ARBHOME)/INCLUDE/awt.hxx
D_main.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
D_main.o: $(ARBHOME)/INCLUDE/awt_preset.hxx
