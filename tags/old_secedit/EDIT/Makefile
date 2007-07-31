# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .depend

CPP_OBJECTS = edit.o edit_naligner.o edit_tools.o ed_conf.o
# edit_mgr_client.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)
.cxx.o:
	$(CPP) $(cflags) -c $(@:.o=.cxx) $(CPPINCLUDES)

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

ed_conf.o: edit.hxx
ed_conf.o: $(ARBHOME)/INCLUDE/ad_prot.h
ed_conf.o: $(ARBHOME)/INCLUDE/adtools.hxx
ed_conf.o: $(ARBHOME)/INCLUDE/arb_assert.h
ed_conf.o: $(ARBHOME)/INCLUDE/arbdb++.hxx
ed_conf.o: $(ARBHOME)/INCLUDE/arbdb.h
ed_conf.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ed_conf.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
ed_conf.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ed_conf.o: $(ARBHOME)/INCLUDE/aw_window.hxx

edit.o: ed_conf.hxx
edit.o: edit.hxx
edit.o: $(ARBHOME)/INCLUDE/ad_prot.h
edit.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
edit.o: $(ARBHOME)/INCLUDE/adtools.hxx
edit.o: $(ARBHOME)/INCLUDE/arb_assert.h
edit.o: $(ARBHOME)/INCLUDE/arbdb++.hxx
edit.o: $(ARBHOME)/INCLUDE/arbdb.h
edit.o: $(ARBHOME)/INCLUDE/arbdbt.h
edit.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
edit.o: $(ARBHOME)/INCLUDE/aw_device.hxx
edit.o: $(ARBHOME)/INCLUDE/AW_helix.hxx
edit.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
edit.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
edit.o: $(ARBHOME)/INCLUDE/aw_root.hxx
edit.o: $(ARBHOME)/INCLUDE/aw_window.hxx
edit.o: $(ARBHOME)/INCLUDE/awt.hxx
edit.o: $(ARBHOME)/INCLUDE/awt_map_key.hxx
edit.o: $(ARBHOME)/INCLUDE/awtc_fast_aligner.hxx
edit.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
edit.o: $(ARBHOME)/INCLUDE/gde.hxx
edit.o: $(ARBHOME)/INCLUDE/st_window.hxx

edit_naligner.o: $(ARBHOME)/INCLUDE/ad_prot.h
edit_naligner.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
edit_naligner.o: $(ARBHOME)/INCLUDE/arb_assert.h
edit_naligner.o: $(ARBHOME)/INCLUDE/arbdb.h
edit_naligner.o: $(ARBHOME)/INCLUDE/arbdbt.h
edit_naligner.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
edit_naligner.o: $(ARBHOME)/INCLUDE/aw_device.hxx
edit_naligner.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
edit_naligner.o: $(ARBHOME)/INCLUDE/aw_root.hxx
edit_naligner.o: $(ARBHOME)/INCLUDE/aw_window.hxx
edit_naligner.o: $(ARBHOME)/INCLUDE/awt.hxx
edit_naligner.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx

edit_tools.o: edit.hxx
edit_tools.o: $(ARBHOME)/INCLUDE/ad_prot.h
edit_tools.o: $(ARBHOME)/INCLUDE/adtools.hxx
edit_tools.o: $(ARBHOME)/INCLUDE/arb_assert.h
edit_tools.o: $(ARBHOME)/INCLUDE/arbdb++.hxx
edit_tools.o: $(ARBHOME)/INCLUDE/arbdb.h
edit_tools.o: $(ARBHOME)/INCLUDE/aw_device.hxx
edit_tools.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
edit_tools.o: $(ARBHOME)/INCLUDE/aw_root.hxx
edit_tools.o: $(ARBHOME)/INCLUDE/aw_window.hxx
