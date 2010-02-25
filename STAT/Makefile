# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

CPP_OBJECTS = \
	ST_ml.o \
	ST_window.o \
	ST_quality.o \

$(MAIN): $(CPP_OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)

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

ST_ml.o: MostLikelySeq.hxx
ST_ml.o: st_ml.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
ST_ml.o: $(ARBHOME)/INCLUDE/ad_prot.h
ST_ml.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ST_ml.o: $(ARBHOME)/INCLUDE/AliView.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/AP_sequence.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/AP_Tree.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/arb_assert.h
ST_ml.o: $(ARBHOME)/INCLUDE/arb_error.h
ST_ml.o: $(ARBHOME)/INCLUDE/ARB_Tree.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/arbdb.h
ST_ml.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ST_ml.o: $(ARBHOME)/INCLUDE/arbdbt.h
ST_ml.o: $(ARBHOME)/INCLUDE/arbtools.h
ST_ml.o: $(ARBHOME)/INCLUDE/attributes.h
ST_ml.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/ColumnStat.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/downcast.h
ST_ml.o: $(ARBHOME)/INCLUDE/dupstr.h
ST_ml.o: $(ARBHOME)/INCLUDE/gui_aliview.hxx
ST_ml.o: $(ARBHOME)/INCLUDE/smartptr.h

ST_quality.o: MostLikelySeq.hxx
ST_quality.o: st_ml.hxx
ST_quality.o: st_quality.hxx
ST_quality.o: st_window.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
ST_quality.o: $(ARBHOME)/INCLUDE/ad_prot.h
ST_quality.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ST_quality.o: $(ARBHOME)/INCLUDE/AliView.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/AP_sequence.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/arb_assert.h
ST_quality.o: $(ARBHOME)/INCLUDE/arb_error.h
ST_quality.o: $(ARBHOME)/INCLUDE/arbdb.h
ST_quality.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ST_quality.o: $(ARBHOME)/INCLUDE/arbdbt.h
ST_quality.o: $(ARBHOME)/INCLUDE/arbtools.h
ST_quality.o: $(ARBHOME)/INCLUDE/attributes.h
ST_quality.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/aw_position.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/ColumnStat.hxx
ST_quality.o: $(ARBHOME)/INCLUDE/dupstr.h
ST_quality.o: $(ARBHOME)/INCLUDE/smartptr.h

ST_window.o: st_ml.hxx
ST_window.o: st_quality.hxx
ST_window.o: st_window.hxx
ST_window.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
ST_window.o: $(ARBHOME)/INCLUDE/ad_prot.h
ST_window.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ST_window.o: $(ARBHOME)/INCLUDE/arb_assert.h
ST_window.o: $(ARBHOME)/INCLUDE/arb_error.h
ST_window.o: $(ARBHOME)/INCLUDE/arbdb.h
ST_window.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ST_window.o: $(ARBHOME)/INCLUDE/arbdbt.h
ST_window.o: $(ARBHOME)/INCLUDE/arbtools.h
ST_window.o: $(ARBHOME)/INCLUDE/attributes.h
ST_window.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ST_window.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ST_window.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
ST_window.o: $(ARBHOME)/INCLUDE/aw_position.hxx
ST_window.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ST_window.o: $(ARBHOME)/INCLUDE/aw_select.hxx
ST_window.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ST_window.o: $(ARBHOME)/INCLUDE/awt.hxx
ST_window.o: $(ARBHOME)/INCLUDE/awt_filter.hxx
ST_window.o: $(ARBHOME)/INCLUDE/awt_item_sel_list.hxx
ST_window.o: $(ARBHOME)/INCLUDE/ColumnStat.hxx
ST_window.o: $(ARBHOME)/INCLUDE/dupstr.h
ST_window.o: $(ARBHOME)/INCLUDE/gui_aliview.hxx
ST_window.o: $(ARBHOME)/INCLUDE/smartptr.h
