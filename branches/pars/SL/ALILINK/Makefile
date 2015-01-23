# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .cxx .depend

OBJECTS=$(subst .cxx,.o,$(wildcard *.cxx))

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

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

AdminGui.o: AliAdmin.h
AdminGui.o: $(ARBHOME)/INCLUDE/ad_prot.h
AdminGui.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AdminGui.o: $(ARBHOME)/INCLUDE/arb_assert.h
AdminGui.o: $(ARBHOME)/INCLUDE/arb_core.h
AdminGui.o: $(ARBHOME)/INCLUDE/arb_error.h
AdminGui.o: $(ARBHOME)/INCLUDE/arb_msg.h
AdminGui.o: $(ARBHOME)/INCLUDE/arb_string.h
AdminGui.o: $(ARBHOME)/INCLUDE/arbdb.h
AdminGui.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AdminGui.o: $(ARBHOME)/INCLUDE/arbdbt.h
AdminGui.o: $(ARBHOME)/INCLUDE/arbtools.h
AdminGui.o: $(ARBHOME)/INCLUDE/attributes.h
AdminGui.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_base.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_question.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
AdminGui.o: $(ARBHOME)/INCLUDE/cb.h
AdminGui.o: $(ARBHOME)/INCLUDE/cb_base.h
AdminGui.o: $(ARBHOME)/INCLUDE/cbtypes.h
AdminGui.o: $(ARBHOME)/INCLUDE/cxxforward.h
AdminGui.o: $(ARBHOME)/INCLUDE/downcast.h
AdminGui.o: $(ARBHOME)/INCLUDE/dupstr.h
AdminGui.o: $(ARBHOME)/INCLUDE/gccver.h
AdminGui.o: $(ARBHOME)/INCLUDE/insdel.h
AdminGui.o: $(ARBHOME)/INCLUDE/smartptr.h
AdminGui.o: $(ARBHOME)/INCLUDE/static_assert.h
AdminGui.o: $(ARBHOME)/INCLUDE/test_global.h
AdminGui.o: $(ARBHOME)/INCLUDE/ttypes.h

AliAdmin.o: AliAdmin.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arb_assert.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arb_core.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arb_error.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arb_msg.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AliAdmin.o: $(ARBHOME)/INCLUDE/arbtools.h
AliAdmin.o: $(ARBHOME)/INCLUDE/attributes.h
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_base.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AliAdmin.o: $(ARBHOME)/INCLUDE/cb.h
AliAdmin.o: $(ARBHOME)/INCLUDE/cb_base.h
AliAdmin.o: $(ARBHOME)/INCLUDE/cbtypes.h
AliAdmin.o: $(ARBHOME)/INCLUDE/cxxforward.h
AliAdmin.o: $(ARBHOME)/INCLUDE/dupstr.h
AliAdmin.o: $(ARBHOME)/INCLUDE/gccver.h
AliAdmin.o: $(ARBHOME)/INCLUDE/smartptr.h
AliAdmin.o: $(ARBHOME)/INCLUDE/static_assert.h
AliAdmin.o: $(ARBHOME)/INCLUDE/test_global.h
AliAdmin.o: $(ARBHOME)/INCLUDE/test_unit.h
AliAdmin.o: $(ARBHOME)/INCLUDE/ttypes.h
