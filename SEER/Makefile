# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend


CPP_OBJECTS =  SEER_main.o SEER_interface.o SEER_connect.o SEER_arb.o SEER_to_seer.o

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

SEER_arb.o: seer.hxx
SEER_arb.o: seer_interface.hxx
SEER_arb.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEER_arb.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
SEER_arb.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEER_arb.o: $(ARBHOME)/INCLUDE/arbdb.h
SEER_arb.o: $(ARBHOME)/INCLUDE/arbdbt.h
SEER_arb.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
SEER_arb.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEER_arb.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
SEER_arb.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SEER_arb.o: $(ARBHOME)/INCLUDE/aw_window.hxx
SEER_arb.o: $(ARBHOME)/INCLUDE/awt.hxx
SEER_arb.o: $(ARBHOME)/INCLUDE/ntree.hxx

SEER_connect.o: seer.hxx
SEER_connect.o: seer_interface.hxx
SEER_connect.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEER_connect.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
SEER_connect.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEER_connect.o: $(ARBHOME)/INCLUDE/arbdb.h
SEER_connect.o: $(ARBHOME)/INCLUDE/arbdbt.h
SEER_connect.o: $(ARBHOME)/INCLUDE/aw_root.hxx

SEER_interface.o: seer_interface.hxx

SEER_main.o: seer.hxx
SEER_main.o: seer_interface.hxx
SEER_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEER_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
SEER_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEER_main.o: $(ARBHOME)/INCLUDE/arbdb.h
SEER_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
SEER_main.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
SEER_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEER_main.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
SEER_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SEER_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
SEER_main.o: $(ARBHOME)/INCLUDE/awt.hxx
SEER_main.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
SEER_main.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
SEER_main.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
SEER_main.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
SEER_main.o: $(ARBHOME)/INCLUDE/ntree.hxx

SEER_to_seer.o: seer.hxx
SEER_to_seer.o: seer_interface.hxx
SEER_to_seer.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEER_to_seer.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
SEER_to_seer.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEER_to_seer.o: $(ARBHOME)/INCLUDE/arbdb.h
SEER_to_seer.o: $(ARBHOME)/INCLUDE/arbdbt.h
SEER_to_seer.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
SEER_to_seer.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEER_to_seer.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
SEER_to_seer.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SEER_to_seer.o: $(ARBHOME)/INCLUDE/aw_window.hxx
SEER_to_seer.o: $(ARBHOME)/INCLUDE/awt.hxx
SEER_to_seer.o: $(ARBHOME)/INCLUDE/ntree.hxx
