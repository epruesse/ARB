# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = \
	GUI_aliview.o \
	ColumnStat.o \
	AWT_filter.o \

OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES) $(POST_COMPILE)

.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES) $(POST_COMPILE)

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

AWT_filter.o: awt_filter.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWT_filter.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWT_filter.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arb_core.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arb_error.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arb_msg.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arb_string.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arbdb.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWT_filter.o: $(ARBHOME)/INCLUDE/arbtools.h
AWT_filter.o: $(ARBHOME)/INCLUDE/attributes.h
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_base.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/awt.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
AWT_filter.o: $(ARBHOME)/INCLUDE/cb.h
AWT_filter.o: $(ARBHOME)/INCLUDE/dupstr.h
AWT_filter.o: $(ARBHOME)/INCLUDE/smartptr.h
AWT_filter.o: $(ARBHOME)/INCLUDE/test_global.h

ColumnStat.o: ColumnStat.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/ad_prot.h
ColumnStat.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ColumnStat.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/arb_assert.h
ColumnStat.o: $(ARBHOME)/INCLUDE/arb_core.h
ColumnStat.o: $(ARBHOME)/INCLUDE/arb_error.h
ColumnStat.o: $(ARBHOME)/INCLUDE/arb_msg.h
ColumnStat.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
ColumnStat.o: $(ARBHOME)/INCLUDE/arb_string.h
ColumnStat.o: $(ARBHOME)/INCLUDE/arbdb.h
ColumnStat.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ColumnStat.o: $(ARBHOME)/INCLUDE/arbdbt.h
ColumnStat.o: $(ARBHOME)/INCLUDE/arbtools.h
ColumnStat.o: $(ARBHOME)/INCLUDE/attributes.h
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_base.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
ColumnStat.o: $(ARBHOME)/INCLUDE/cb.h
ColumnStat.o: $(ARBHOME)/INCLUDE/dupstr.h
ColumnStat.o: $(ARBHOME)/INCLUDE/smartptr.h
ColumnStat.o: $(ARBHOME)/INCLUDE/test_global.h

GUI_aliview.o: awt_filter.hxx
GUI_aliview.o: ColumnStat.hxx
GUI_aliview.o: gui_aliview.hxx
GUI_aliview.o: $(ARBHOME)/INCLUDE/AliView.hxx
GUI_aliview.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
GUI_aliview.o: $(ARBHOME)/INCLUDE/arb_assert.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/arb_core.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/arbdb_base.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/dupstr.h
GUI_aliview.o: $(ARBHOME)/INCLUDE/test_global.h
