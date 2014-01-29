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

TreeAdmin.o: TreeAdmin.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/ad_prot.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arb_assert.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arb_core.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arb_error.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arb_global_defs.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arb_msg.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arb_string.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arbdb.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arbdb_base.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arbdbt.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/arbtools.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/attributes.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_base.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_root.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/aw_window.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/awt_TreeAwars.hxx
TreeAdmin.o: $(ARBHOME)/INCLUDE/cb.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/cb_base.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/cbtypes.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/cxxforward.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/downcast.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/dupstr.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/gccver.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/smartptr.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/static_assert.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/test_global.h
TreeAdmin.o: $(ARBHOME)/INCLUDE/ttypes.h
