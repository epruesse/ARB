# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .depend

CPP_OBJECTS =   GDE_ParseMenu.o GDE.o GDE_event.o \
	GDE_arbdb_io.o  GDE_HGLfile.o\
	GDE_Genbank.o GDE_FileIO.o

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

GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/arbdb.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/arb_assert.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/ad_prot.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/awt.hxx gde.hxx GDE_menu.h
GDE_ParseMenu.o: GDE_def.h GDE_global.h GDE_extglob.h

GDE.o: $(ARBHOME)/INCLUDE/arbdb.h
GDE.o: $(ARBHOME)/INCLUDE/arb_assert.h
GDE.o: $(ARBHOME)/INCLUDE/ad_prot.h $(ARBHOME)/INCLUDE/arbdbt.h
GDE.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GDE.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GDE.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GDE.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GDE.o: $(ARBHOME)/INCLUDE/awt.hxx gde.hxx GDE_menu.h GDE_def.h
GDE.o: GDE_extglob.h

GDE_event.o: $(ARBHOME)/INCLUDE/arbdb.h
GDE_event.o: $(ARBHOME)/INCLUDE/arb_assert.h
GDE_event.o: $(ARBHOME)/INCLUDE/ad_prot.h
GDE_event.o: $(ARBHOME)/INCLUDE/arbdbt.h
GDE_event.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GDE_event.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GDE_event.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GDE_event.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GDE_event.o: $(ARBHOME)/INCLUDE/awt.hxx
GDE_event.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
GDE_event.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
GDE_event.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx gde.hxx
GDE_event.o: GDE_menu.h GDE_def.h GDE_extglob.h

GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/arbdb.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/arb_assert.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/ad_prot.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/arbdbt.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/awt.hxx
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx gde.hxx
GDE_arbdb_io.o: GDE_def.h GDE_menu.h GDE_extglob.h

GDE_HGLfile.o: $(ARBHOME)/INCLUDE/arbdb.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/arb_assert.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/ad_prot.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/awt.hxx gde.hxx GDE_menu.h GDE_def.h
GDE_HGLfile.o: GDE_extglob.h

GDE_Genbank.o: $(ARBHOME)/INCLUDE/arbdb.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/arb_assert.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/ad_prot.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GDE_Genbank.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GDE_Genbank.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GDE_Genbank.o: $(ARBHOME)/INCLUDE/awt.hxx gde.hxx GDE_menu.h GDE_def.h
GDE_Genbank.o: GDE_extglob.h

GDE_FileIO.o: $(ARBHOME)/INCLUDE/arbdb.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/arb_assert.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/ad_prot.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/arbdbt.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GDE_FileIO.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GDE_FileIO.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GDE_FileIO.o: $(ARBHOME)/INCLUDE/awt.hxx gde.hxx GDE_def.h GDE_menu.h
GDE_FileIO.o: GDE_extglob.h
