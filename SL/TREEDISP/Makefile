# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .cxx .hxx .depend

C_OBJECTS = 
CPP_OBJECTS = \
	TreeDisplay.o \
	irstree_display.o \
	TreeCallbacks.o \

OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES)

proto:
	../../MAKEBIN/aisc_mkpt -P -w TreeCallbacks.hxx TreeCallbacks.cxx >TreeCallbacks.hxx.tmp
	../../SOURCE_TOOLS/mv_if_diff TreeCallbacks.hxx.tmp TreeCallbacks.hxx

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

irstree_display.o: TreeDisplay.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
irstree_display.o: $(ARBHOME)/INCLUDE/ad_prot.h
irstree_display.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
irstree_display.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/AP_sequence.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/AP_Tree.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/arb_assert.h
irstree_display.o: $(ARBHOME)/INCLUDE/ARB_Tree.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/arbdb.h
irstree_display.o: $(ARBHOME)/INCLUDE/arbdb_base.h
irstree_display.o: $(ARBHOME)/INCLUDE/arbdbt.h
irstree_display.o: $(ARBHOME)/INCLUDE/arbtools.h
irstree_display.o: $(ARBHOME)/INCLUDE/attributes.h
irstree_display.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/aw_device.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/aw_position.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/aw_root.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/aw_window.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
irstree_display.o: $(ARBHOME)/INCLUDE/awt_nds.hxx

TreeCallbacks.o: TreeCallbacks.hxx
TreeCallbacks.o: TreeDisplay.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/ad_prot.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/AP_sequence.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/AP_Tree.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/arb_assert.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/ARB_Tree.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/arbdb.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/arbdb_base.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/arbdbt.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/arbtools.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/attributes.h
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_device.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_position.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_root.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/aw_window.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/awt.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/awt_advice.hxx
TreeCallbacks.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx

TreeDisplay.o: TreeDisplay.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/ad_prot.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/AP_sequence.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/AP_Tree.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/arb_assert.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/ARB_Tree.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/arbdb.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/arbdb_base.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/arbdbt.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/arbtools.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/attributes.h
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_device.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_position.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_root.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/aw_window.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/awt.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/awt_attributes.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
TreeDisplay.o: $(ARBHOME)/INCLUDE/awt_nds.hxx
