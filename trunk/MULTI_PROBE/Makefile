# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend


CPP_OBJECTS =  MP_main.o MP_Window.o MP_noclass.o MP_sonde.o MP_sondentopf.o MP_mo_liste.o MP_probe.o \
		MP_GenerationDuplicates.o MP_Generation.o MP_probe_combi_statistic.o MP_probe_tabs.o \
		MP_permute.o


$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

andy:
	CC -o mpw $(CPP_OBJECTS)

clean:
	rm *.o
	rm *.a
	rm *.bak
	rm *~
	rm *#
	rm *\%

#.IGNORE:
#	clean

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

MP_Generation.o: MP_externs.hxx
MP_Generation.o: MP_probe.hxx
MP_Generation.o: mpdefs.h
MP_Generation.o: MultiProbe.hxx
MP_Generation.o: SoTl.hxx
MP_Generation.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_Generation.o: $(ARBHOME)/INCLUDE/arb_assert.h
MP_Generation.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_Generation.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_Generation.o: $(ARBHOME)/INCLUDE/client.h
MP_Generation.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_Generation.o: $(ARBHOME)/INCLUDE/servercntrl.h

MP_GenerationDuplicates.o: MP_externs.hxx
MP_GenerationDuplicates.o: MP_probe.hxx
MP_GenerationDuplicates.o: mpdefs.h
MP_GenerationDuplicates.o: MultiProbe.hxx
MP_GenerationDuplicates.o: SoTl.hxx
MP_GenerationDuplicates.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_GenerationDuplicates.o: $(ARBHOME)/INCLUDE/arb_assert.h
MP_GenerationDuplicates.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_GenerationDuplicates.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_GenerationDuplicates.o: $(ARBHOME)/INCLUDE/client.h
MP_GenerationDuplicates.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_GenerationDuplicates.o: $(ARBHOME)/INCLUDE/servercntrl.h

MP_main.o: MP_externs.hxx
MP_main.o: MP_probe.hxx
MP_main.o: mp_proto.hxx
MP_main.o: mpdefs.h
MP_main.o: MultiProbe.hxx
MP_main.o: SoTl.hxx
MP_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
MP_main.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_main.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MP_main.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
MP_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MP_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MP_main.o: $(ARBHOME)/INCLUDE/client.h
MP_main.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_main.o: $(ARBHOME)/INCLUDE/servercntrl.h

MP_mo_liste.o: MP_externs.hxx
MP_mo_liste.o: MP_probe.hxx
MP_mo_liste.o: mpdefs.h
MP_mo_liste.o: MultiProbe.hxx
MP_mo_liste.o: SoTl.hxx
MP_mo_liste.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_mo_liste.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MP_mo_liste.o: $(ARBHOME)/INCLUDE/arb_assert.h
MP_mo_liste.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_mo_liste.o: $(ARBHOME)/INCLUDE/arbdbt.h
MP_mo_liste.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_mo_liste.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MP_mo_liste.o: $(ARBHOME)/INCLUDE/client.h
MP_mo_liste.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_mo_liste.o: $(ARBHOME)/INCLUDE/servercntrl.h

MP_noclass.o: MP_externs.hxx
MP_noclass.o: MP_probe.hxx
MP_noclass.o: mp_proto.hxx
MP_noclass.o: mpdefs.h
MP_noclass.o: MultiProbe.hxx
MP_noclass.o: SoTl.hxx
MP_noclass.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_noclass.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MP_noclass.o: $(ARBHOME)/INCLUDE/arb_assert.h
MP_noclass.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_noclass.o: $(ARBHOME)/INCLUDE/arbdbt.h
MP_noclass.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_noclass.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
MP_noclass.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MP_noclass.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
MP_noclass.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MP_noclass.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MP_noclass.o: $(ARBHOME)/INCLUDE/awt.hxx
MP_noclass.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
MP_noclass.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
MP_noclass.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
MP_noclass.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
MP_noclass.o: $(ARBHOME)/INCLUDE/awt_tree_cb.hxx
MP_noclass.o: $(ARBHOME)/INCLUDE/client.h
MP_noclass.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_noclass.o: $(ARBHOME)/INCLUDE/servercntrl.h

MP_permute.o: MP_externs.hxx
MP_permute.o: MP_probe.hxx
MP_permute.o: mpdefs.h
MP_permute.o: MultiProbe.hxx
MP_permute.o: SoTl.hxx
MP_permute.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_permute.o: $(ARBHOME)/INCLUDE/arb_assert.h
MP_permute.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_permute.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_permute.o: $(ARBHOME)/INCLUDE/client.h
MP_permute.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_permute.o: $(ARBHOME)/INCLUDE/servercntrl.h

MP_probe.o: MP_externs.hxx
MP_probe.o: MP_probe.hxx
MP_probe.o: mpdefs.h
MP_probe.o: MultiProbe.hxx
MP_probe.o: SoTl.hxx
MP_probe.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_probe.o: $(ARBHOME)/INCLUDE/arb_assert.h
MP_probe.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_probe.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_probe.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MP_probe.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
MP_probe.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MP_probe.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MP_probe.o: $(ARBHOME)/INCLUDE/client.h
MP_probe.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_probe.o: $(ARBHOME)/INCLUDE/servercntrl.h

MP_probe_combi_statistic.o: MP_externs.hxx
MP_probe_combi_statistic.o: MP_probe.hxx
MP_probe_combi_statistic.o: mpdefs.h
MP_probe_combi_statistic.o: MultiProbe.hxx
MP_probe_combi_statistic.o: SoTl.hxx
MP_probe_combi_statistic.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_probe_combi_statistic.o: $(ARBHOME)/INCLUDE/arb_assert.h
MP_probe_combi_statistic.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_probe_combi_statistic.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_probe_combi_statistic.o: $(ARBHOME)/INCLUDE/client.h
MP_probe_combi_statistic.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_probe_combi_statistic.o: $(ARBHOME)/INCLUDE/servercntrl.h

MP_probe_tabs.o: MP_externs.hxx
MP_probe_tabs.o: MP_probe.hxx
MP_probe_tabs.o: mpdefs.h
MP_probe_tabs.o: MultiProbe.hxx
MP_probe_tabs.o: SoTl.hxx
MP_probe_tabs.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_probe_tabs.o: $(ARBHOME)/INCLUDE/arb_assert.h
MP_probe_tabs.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_probe_tabs.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_probe_tabs.o: $(ARBHOME)/INCLUDE/client.h
MP_probe_tabs.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_probe_tabs.o: $(ARBHOME)/INCLUDE/servercntrl.h

MP_sonde.o: MP_externs.hxx
MP_sonde.o: MP_probe.hxx
MP_sonde.o: mpdefs.h
MP_sonde.o: MultiProbe.hxx
MP_sonde.o: SoTl.hxx
MP_sonde.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_sonde.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MP_sonde.o: $(ARBHOME)/INCLUDE/arb_assert.h
MP_sonde.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_sonde.o: $(ARBHOME)/INCLUDE/arbdbt.h
MP_sonde.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_sonde.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
MP_sonde.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MP_sonde.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
MP_sonde.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
MP_sonde.o: $(ARBHOME)/INCLUDE/client.h
MP_sonde.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_sonde.o: $(ARBHOME)/INCLUDE/servercntrl.h

MP_sondentopf.o: MP_externs.hxx
MP_sondentopf.o: MP_probe.hxx
MP_sondentopf.o: mpdefs.h
MP_sondentopf.o: MultiProbe.hxx
MP_sondentopf.o: SoTl.hxx
MP_sondentopf.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_sondentopf.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MP_sondentopf.o: $(ARBHOME)/INCLUDE/arb_assert.h
MP_sondentopf.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_sondentopf.o: $(ARBHOME)/INCLUDE/arbdbt.h
MP_sondentopf.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_sondentopf.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
MP_sondentopf.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MP_sondentopf.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
MP_sondentopf.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
MP_sondentopf.o: $(ARBHOME)/INCLUDE/client.h
MP_sondentopf.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_sondentopf.o: $(ARBHOME)/INCLUDE/servercntrl.h

MP_Window.o: MP_externs.hxx
MP_Window.o: MP_probe.hxx
MP_Window.o: mp_proto.hxx
MP_Window.o: mpdefs.h
MP_Window.o: MultiProbe.hxx
MP_Window.o: SoTl.hxx
MP_Window.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_Window.o: $(ARBHOME)/INCLUDE/arb_assert.h
MP_Window.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_Window.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_Window.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MP_Window.o: $(ARBHOME)/INCLUDE/aw_global.hxx
MP_Window.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
MP_Window.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MP_Window.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MP_Window.o: $(ARBHOME)/INCLUDE/awt.hxx
MP_Window.o: $(ARBHOME)/INCLUDE/client.h
MP_Window.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_Window.o: $(ARBHOME)/INCLUDE/servercntrl.h
