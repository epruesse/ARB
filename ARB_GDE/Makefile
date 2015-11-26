# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt
.SUFFIXES: .o .cxx .depend

CPP_OBJECTS =   \
	GDE.o \
	GDE_FileIO.o \
	GDE_Genbank.o \
	GDE_HGLfile.o \
	GDE_ParseMenu.o \
	GDE_arbdb_io.o \
	GDE_event.o \
	GDE_global.o \

CPP_FILES=$(CPP_OBJECTS:.o=.cxx)

$(MAIN): $(CPP_OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)
.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

GDE_proto.h: $(CPP_FILES) Makefile
	$(MAKE) proto

proto:
	../AISC_MKPTPS/aisc_mkpt -P -G -E -w GDE_proto.h $(CPP_FILES) > GDE_proto.h.tmp
	../SOURCE_TOOLS/mv_if_diff GDE_proto.h.tmp GDE_proto.h

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

GDE.o: gde.hxx
GDE.o: GDE_awars.h
GDE.o: GDE_def.h
GDE.o: GDE_extglob.h
GDE.o: GDE_menu.h
GDE.o: GDE_proto.h
GDE.o: $(ARBHOME)/INCLUDE/ad_prot.h
GDE.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GDE.o: $(ARBHOME)/INCLUDE/arb_assert.h
GDE.o: $(ARBHOME)/INCLUDE/arb_core.h
GDE.o: $(ARBHOME)/INCLUDE/arb_error.h
GDE.o: $(ARBHOME)/INCLUDE/arb_msg.h
GDE.o: $(ARBHOME)/INCLUDE/arb_str.h
GDE.o: $(ARBHOME)/INCLUDE/arb_string.h
GDE.o: $(ARBHOME)/INCLUDE/arbdb.h
GDE.o: $(ARBHOME)/INCLUDE/arbdb_base.h
GDE.o: $(ARBHOME)/INCLUDE/arbdbt.h
GDE.o: $(ARBHOME)/INCLUDE/arbtools.h
GDE.o: $(ARBHOME)/INCLUDE/attributes.h
GDE.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
GDE.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
GDE.o: $(ARBHOME)/INCLUDE/aw_base.hxx
GDE.o: $(ARBHOME)/INCLUDE/aw_file.hxx
GDE.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
GDE.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
GDE.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GDE.o: $(ARBHOME)/INCLUDE/aw_scalar.hxx
GDE.o: $(ARBHOME)/INCLUDE/aw_select.hxx
GDE.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GDE.o: $(ARBHOME)/INCLUDE/awt_config_manager.hxx
GDE.o: $(ARBHOME)/INCLUDE/awt_filter.hxx
GDE.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
GDE.o: $(ARBHOME)/INCLUDE/cb.h
GDE.o: $(ARBHOME)/INCLUDE/cb_base.h
GDE.o: $(ARBHOME)/INCLUDE/cbtypes.h
GDE.o: $(ARBHOME)/INCLUDE/cxxforward.h
GDE.o: $(ARBHOME)/INCLUDE/downcast.h
GDE.o: $(ARBHOME)/INCLUDE/dupstr.h
GDE.o: $(ARBHOME)/INCLUDE/gccver.h
GDE.o: $(ARBHOME)/INCLUDE/smartptr.h
GDE.o: $(ARBHOME)/INCLUDE/static_assert.h
GDE.o: $(ARBHOME)/INCLUDE/test_global.h
GDE.o: $(ARBHOME)/INCLUDE/ttypes.h

GDE_arbdb_io.o: gde.hxx
GDE_arbdb_io.o: GDE_def.h
GDE_arbdb_io.o: GDE_extglob.h
GDE_arbdb_io.o: GDE_menu.h
GDE_arbdb_io.o: GDE_proto.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/ad_prot.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/arb_assert.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/arb_core.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/arb_error.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/arb_msg.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/arb_progress.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/arb_string.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/arbdb.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/arbdb_base.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/arbdbt.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/arbtools.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/attributes.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/aw_base.hxx
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/cb.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/cb_base.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/cbtypes.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/cxxforward.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/downcast.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/dupstr.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/gccver.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/smartptr.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/static_assert.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/test_global.h
GDE_arbdb_io.o: $(ARBHOME)/INCLUDE/ttypes.h

GDE_event.o: gde.hxx
GDE_event.o: GDE_awars.h
GDE_event.o: GDE_def.h
GDE_event.o: GDE_extglob.h
GDE_event.o: GDE_menu.h
GDE_event.o: GDE_proto.h
GDE_event.o: $(ARBHOME)/INCLUDE/ad_prot.h
GDE_event.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GDE_event.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
GDE_event.o: $(ARBHOME)/INCLUDE/arb_assert.h
GDE_event.o: $(ARBHOME)/INCLUDE/arb_core.h
GDE_event.o: $(ARBHOME)/INCLUDE/arb_error.h
GDE_event.o: $(ARBHOME)/INCLUDE/arb_msg.h
GDE_event.o: $(ARBHOME)/INCLUDE/arb_progress.h
GDE_event.o: $(ARBHOME)/INCLUDE/arb_string.h
GDE_event.o: $(ARBHOME)/INCLUDE/arbdb.h
GDE_event.o: $(ARBHOME)/INCLUDE/arbdb_base.h
GDE_event.o: $(ARBHOME)/INCLUDE/arbdbt.h
GDE_event.o: $(ARBHOME)/INCLUDE/arbtools.h
GDE_event.o: $(ARBHOME)/INCLUDE/attributes.h
GDE_event.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
GDE_event.o: $(ARBHOME)/INCLUDE/aw_base.hxx
GDE_event.o: $(ARBHOME)/INCLUDE/aw_file.hxx
GDE_event.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
GDE_event.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
GDE_event.o: $(ARBHOME)/INCLUDE/aw_question.hxx
GDE_event.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GDE_event.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GDE_event.o: $(ARBHOME)/INCLUDE/awt_filter.hxx
GDE_event.o: $(ARBHOME)/INCLUDE/cb.h
GDE_event.o: $(ARBHOME)/INCLUDE/cb_base.h
GDE_event.o: $(ARBHOME)/INCLUDE/cbtypes.h
GDE_event.o: $(ARBHOME)/INCLUDE/cxxforward.h
GDE_event.o: $(ARBHOME)/INCLUDE/downcast.h
GDE_event.o: $(ARBHOME)/INCLUDE/dupstr.h
GDE_event.o: $(ARBHOME)/INCLUDE/gccver.h
GDE_event.o: $(ARBHOME)/INCLUDE/smartptr.h
GDE_event.o: $(ARBHOME)/INCLUDE/static_assert.h
GDE_event.o: $(ARBHOME)/INCLUDE/test_global.h
GDE_event.o: $(ARBHOME)/INCLUDE/test_unit.h
GDE_event.o: $(ARBHOME)/INCLUDE/ttypes.h

GDE_FileIO.o: gde.hxx
GDE_FileIO.o: GDE_def.h
GDE_FileIO.o: GDE_extglob.h
GDE_FileIO.o: GDE_menu.h
GDE_FileIO.o: GDE_proto.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/ad_prot.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/arb_assert.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/arb_core.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/arb_error.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/arb_msg.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/arb_string.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/arbdb.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/arbdb_base.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/arbdbt.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/arbtools.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/attributes.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/aw_base.hxx
GDE_FileIO.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
GDE_FileIO.o: $(ARBHOME)/INCLUDE/cb.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/cb_base.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/cbtypes.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/cxxforward.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/downcast.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/dupstr.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/gccver.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/smartptr.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/static_assert.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/test_global.h
GDE_FileIO.o: $(ARBHOME)/INCLUDE/ttypes.h

GDE_Genbank.o: gde.hxx
GDE_Genbank.o: GDE_def.h
GDE_Genbank.o: GDE_extglob.h
GDE_Genbank.o: GDE_menu.h
GDE_Genbank.o: GDE_proto.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/ad_prot.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/arb_assert.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/arb_core.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/arb_error.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/arb_msg.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/arb_string.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/arbdb.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/arbdb_base.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/arbdbt.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/arbtools.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/attributes.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/aw_base.hxx
GDE_Genbank.o: $(ARBHOME)/INCLUDE/cb.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/cb_base.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/cbtypes.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/cxxforward.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/downcast.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/dupstr.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/gccver.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/smartptr.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/static_assert.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/test_global.h
GDE_Genbank.o: $(ARBHOME)/INCLUDE/ttypes.h

GDE_HGLfile.o: gde.hxx
GDE_HGLfile.o: GDE_def.h
GDE_HGLfile.o: GDE_extglob.h
GDE_HGLfile.o: GDE_menu.h
GDE_HGLfile.o: GDE_proto.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/ad_prot.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/arb_assert.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/arb_core.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/arb_error.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/arb_msg.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/arb_string.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/arbdb.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/arbdb_base.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/arbdbt.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/arbtools.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/attributes.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/aw_base.hxx
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/cb.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/cb_base.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/cbtypes.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/cxxforward.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/downcast.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/dupstr.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/gccver.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/smartptr.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/static_assert.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/test_global.h
GDE_HGLfile.o: $(ARBHOME)/INCLUDE/ttypes.h

GDE_ParseMenu.o: gde.hxx
GDE_ParseMenu.o: GDE_def.h
GDE_ParseMenu.o: GDE_extglob.h
GDE_ParseMenu.o: GDE_menu.h
GDE_ParseMenu.o: GDE_proto.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/ad_prot.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/arb_assert.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/arb_core.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/arb_error.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/arb_file.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/arb_msg.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/arb_strarray.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/arb_string.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/arbdb.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/arbdb_base.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/arbdbt.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/arbtools.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/attributes.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/aw_base.hxx
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/cb.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/cb_base.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/cbtypes.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/cxxforward.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/downcast.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/dupstr.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/gccver.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/MultiFileReader.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/smartptr.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/static_assert.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/test_global.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/test_unit.h
GDE_ParseMenu.o: $(ARBHOME)/INCLUDE/ttypes.h
