# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = \
	GUI_aliview.o \
	AWT_csp.o \
	AWT_filter.o \

OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES)

clean:
	rm -f $(OBJECTS) *.a

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

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

AWT_csp.o: awt_csp.hxx
AWT_csp.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_csp.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_csp.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_csp.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
AWT_csp.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_csp.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_csp.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_csp.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_csp.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_csp.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_csp.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_csp.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_csp.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_csp.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_csp.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_csp.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_csp.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
AWT_csp.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
AWT_csp.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_csp.o: $(ARBHOME)/INCLUDE/smartptr.h

AWT_filter.o: awt_filter.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AWT_filter.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_filter.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_filter.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_filter.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/awt.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_filter.o: $(ARBHOME)/INCLUDE/smartptr.h

GUI_aliview.o: awt_csp.hxx
GUI_aliview.o: awt_filter.hxx
GUI_aliview.o: gui_aliview.hxx
GUI_aliview.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/ad_prot.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/AliView.hxx
GUI_aliview.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
GUI_aliview.o: $(ARBHOME)/INCLUDE/arb_assert.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/arb_error.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/arbdb.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/arbdb_base.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/arbdbt.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/arbtools.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/attributes.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/dupstr.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/smartptr.h
