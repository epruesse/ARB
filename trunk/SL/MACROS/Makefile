# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .cxx .depend

OBJECTS=$(subst .cxx,.o,$(wildcard *.cxx))

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE)

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from SL)

dbserver.o: dbserver.hxx
dbserver.o: macros.hxx
dbserver.o: macros_local.hxx
dbserver.o: trackers.hxx
dbserver.o: $(ARBHOME)/INCLUDE/ad_prot.h
dbserver.o: $(ARBHOME)/INCLUDE/ad_remote.h
dbserver.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
dbserver.o: $(ARBHOME)/INCLUDE/arb_assert.h
dbserver.o: $(ARBHOME)/INCLUDE/arb_core.h
dbserver.o: $(ARBHOME)/INCLUDE/arb_error.h
dbserver.o: $(ARBHOME)/INCLUDE/arb_msg.h
dbserver.o: $(ARBHOME)/INCLUDE/arb_sleep.h
dbserver.o: $(ARBHOME)/INCLUDE/arb_string.h
dbserver.o: $(ARBHOME)/INCLUDE/arbdb.h
dbserver.o: $(ARBHOME)/INCLUDE/arbdb_base.h
dbserver.o: $(ARBHOME)/INCLUDE/arbdbt.h
dbserver.o: $(ARBHOME)/INCLUDE/arbtools.h
dbserver.o: $(ARBHOME)/INCLUDE/attributes.h
dbserver.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
dbserver.o: $(ARBHOME)/INCLUDE/aw_base.hxx
dbserver.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
dbserver.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
dbserver.o: $(ARBHOME)/INCLUDE/aw_root.hxx
dbserver.o: $(ARBHOME)/INCLUDE/aw_window.hxx
dbserver.o: $(ARBHOME)/INCLUDE/cb.h
dbserver.o: $(ARBHOME)/INCLUDE/cb_base.h
dbserver.o: $(ARBHOME)/INCLUDE/cbtypes.h
dbserver.o: $(ARBHOME)/INCLUDE/cxxforward.h
dbserver.o: $(ARBHOME)/INCLUDE/downcast.h
dbserver.o: $(ARBHOME)/INCLUDE/dupstr.h
dbserver.o: $(ARBHOME)/INCLUDE/gccver.h
dbserver.o: $(ARBHOME)/INCLUDE/smartptr.h
dbserver.o: $(ARBHOME)/INCLUDE/static_assert.h
dbserver.o: $(ARBHOME)/INCLUDE/test_global.h
dbserver.o: $(ARBHOME)/INCLUDE/ttypes.h

macro_gui.o: macros.hxx
macro_gui.o: macros_local.hxx
macro_gui.o: trackers.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/ad_cb.h
macro_gui.o: $(ARBHOME)/INCLUDE/ad_cb_prot.h
macro_gui.o: $(ARBHOME)/INCLUDE/ad_prot.h
macro_gui.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
macro_gui.o: $(ARBHOME)/INCLUDE/arb_assert.h
macro_gui.o: $(ARBHOME)/INCLUDE/arb_core.h
macro_gui.o: $(ARBHOME)/INCLUDE/arb_error.h
macro_gui.o: $(ARBHOME)/INCLUDE/arb_file.h
macro_gui.o: $(ARBHOME)/INCLUDE/arb_msg.h
macro_gui.o: $(ARBHOME)/INCLUDE/arb_string.h
macro_gui.o: $(ARBHOME)/INCLUDE/arbdb.h
macro_gui.o: $(ARBHOME)/INCLUDE/arbdb_base.h
macro_gui.o: $(ARBHOME)/INCLUDE/arbdbt.h
macro_gui.o: $(ARBHOME)/INCLUDE/arbtools.h
macro_gui.o: $(ARBHOME)/INCLUDE/attributes.h
macro_gui.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_base.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_edit.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_file.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_root.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/aw_window.hxx
macro_gui.o: $(ARBHOME)/INCLUDE/cb.h
macro_gui.o: $(ARBHOME)/INCLUDE/cb_base.h
macro_gui.o: $(ARBHOME)/INCLUDE/cbtypes.h
macro_gui.o: $(ARBHOME)/INCLUDE/cxxforward.h
macro_gui.o: $(ARBHOME)/INCLUDE/downcast.h
macro_gui.o: $(ARBHOME)/INCLUDE/dupstr.h
macro_gui.o: $(ARBHOME)/INCLUDE/gccver.h
macro_gui.o: $(ARBHOME)/INCLUDE/smartptr.h
macro_gui.o: $(ARBHOME)/INCLUDE/static_assert.h
macro_gui.o: $(ARBHOME)/INCLUDE/test_global.h
macro_gui.o: $(ARBHOME)/INCLUDE/ttypes.h

recmac.o: macros_local.hxx
recmac.o: recmac.hxx
recmac.o: $(ARBHOME)/INCLUDE/ad_prot.h
recmac.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
recmac.o: $(ARBHOME)/INCLUDE/arb_assert.h
recmac.o: $(ARBHOME)/INCLUDE/arb_core.h
recmac.o: $(ARBHOME)/INCLUDE/arb_defs.h
recmac.o: $(ARBHOME)/INCLUDE/arb_diff.h
recmac.o: $(ARBHOME)/INCLUDE/arb_error.h
recmac.o: $(ARBHOME)/INCLUDE/arb_file.h
recmac.o: $(ARBHOME)/INCLUDE/arb_msg.h
recmac.o: $(ARBHOME)/INCLUDE/arb_str.h
recmac.o: $(ARBHOME)/INCLUDE/arb_strarray.h
recmac.o: $(ARBHOME)/INCLUDE/arb_string.h
recmac.o: $(ARBHOME)/INCLUDE/arbdb.h
recmac.o: $(ARBHOME)/INCLUDE/arbdb_base.h
recmac.o: $(ARBHOME)/INCLUDE/arbdbt.h
recmac.o: $(ARBHOME)/INCLUDE/arbtools.h
recmac.o: $(ARBHOME)/INCLUDE/attributes.h
recmac.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
recmac.o: $(ARBHOME)/INCLUDE/aw_base.hxx
recmac.o: $(ARBHOME)/INCLUDE/aw_file.hxx
recmac.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
recmac.o: $(ARBHOME)/INCLUDE/aw_root.hxx
recmac.o: $(ARBHOME)/INCLUDE/cb.h
recmac.o: $(ARBHOME)/INCLUDE/cb_base.h
recmac.o: $(ARBHOME)/INCLUDE/cbtypes.h
recmac.o: $(ARBHOME)/INCLUDE/cxxforward.h
recmac.o: $(ARBHOME)/INCLUDE/downcast.h
recmac.o: $(ARBHOME)/INCLUDE/dupstr.h
recmac.o: $(ARBHOME)/INCLUDE/FileContent.h
recmac.o: $(ARBHOME)/INCLUDE/gccver.h
recmac.o: $(ARBHOME)/INCLUDE/smartptr.h
recmac.o: $(ARBHOME)/INCLUDE/static_assert.h
recmac.o: $(ARBHOME)/INCLUDE/test_global.h
recmac.o: $(ARBHOME)/INCLUDE/test_unit.h
recmac.o: $(ARBHOME)/INCLUDE/ttypes.h

trackers.o: dbserver.hxx
trackers.o: macros.hxx
trackers.o: macros_local.hxx
trackers.o: recmac.hxx
trackers.o: trackers.hxx
trackers.o: $(ARBHOME)/INCLUDE/ad_cb.h
trackers.o: $(ARBHOME)/INCLUDE/ad_cb_prot.h
trackers.o: $(ARBHOME)/INCLUDE/ad_prot.h
trackers.o: $(ARBHOME)/INCLUDE/ad_remote.h
trackers.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
trackers.o: $(ARBHOME)/INCLUDE/arb_assert.h
trackers.o: $(ARBHOME)/INCLUDE/arb_core.h
trackers.o: $(ARBHOME)/INCLUDE/arb_error.h
trackers.o: $(ARBHOME)/INCLUDE/arb_msg.h
trackers.o: $(ARBHOME)/INCLUDE/arb_sleep.h
trackers.o: $(ARBHOME)/INCLUDE/arb_strarray.h
trackers.o: $(ARBHOME)/INCLUDE/arb_string.h
trackers.o: $(ARBHOME)/INCLUDE/arbdb.h
trackers.o: $(ARBHOME)/INCLUDE/arbdb_base.h
trackers.o: $(ARBHOME)/INCLUDE/arbdbt.h
trackers.o: $(ARBHOME)/INCLUDE/arbtools.h
trackers.o: $(ARBHOME)/INCLUDE/attributes.h
trackers.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
trackers.o: $(ARBHOME)/INCLUDE/aw_base.hxx
trackers.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
trackers.o: $(ARBHOME)/INCLUDE/aw_root.hxx
trackers.o: $(ARBHOME)/INCLUDE/cb.h
trackers.o: $(ARBHOME)/INCLUDE/cb_base.h
trackers.o: $(ARBHOME)/INCLUDE/cbtypes.h
trackers.o: $(ARBHOME)/INCLUDE/cxxforward.h
trackers.o: $(ARBHOME)/INCLUDE/downcast.h
trackers.o: $(ARBHOME)/INCLUDE/dupstr.h
trackers.o: $(ARBHOME)/INCLUDE/gccver.h
trackers.o: $(ARBHOME)/INCLUDE/smartptr.h
trackers.o: $(ARBHOME)/INCLUDE/static_assert.h
trackers.o: $(ARBHOME)/INCLUDE/test_global.h
trackers.o: $(ARBHOME)/INCLUDE/ttypes.h
