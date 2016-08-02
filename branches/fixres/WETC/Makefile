# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .cxx .depend

CPP_OBJECTS = WETC_main.o

$(MAIN): $(CPP_OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c -o $@ $(@:.o=.cxx)  $(CXX_INCLUDES) $(POST_COMPILE) $<

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

WETC_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
WETC_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
WETC_main.o: $(ARBHOME)/INCLUDE/arb_core.h
WETC_main.o: $(ARBHOME)/INCLUDE/arb_error.h
WETC_main.o: $(ARBHOME)/INCLUDE/arb_mem.h
WETC_main.o: $(ARBHOME)/INCLUDE/arb_msg.h
WETC_main.o: $(ARBHOME)/INCLUDE/arb_string.h
WETC_main.o: $(ARBHOME)/INCLUDE/arbdb.h
WETC_main.o: $(ARBHOME)/INCLUDE/arbdb_base.h
WETC_main.o: $(ARBHOME)/INCLUDE/arbtools.h
WETC_main.o: $(ARBHOME)/INCLUDE/attributes.h
WETC_main.o: $(ARBHOME)/INCLUDE/aw_base.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/awt.hxx
WETC_main.o: $(ARBHOME)/INCLUDE/cb.h
WETC_main.o: $(ARBHOME)/INCLUDE/cb_base.h
WETC_main.o: $(ARBHOME)/INCLUDE/cbtypes.h
WETC_main.o: $(ARBHOME)/INCLUDE/cxxforward.h
WETC_main.o: $(ARBHOME)/INCLUDE/dupstr.h
WETC_main.o: $(ARBHOME)/INCLUDE/gccver.h
WETC_main.o: $(ARBHOME)/INCLUDE/smartptr.h
WETC_main.o: $(ARBHOME)/INCLUDE/static_assert.h
WETC_main.o: $(ARBHOME)/INCLUDE/test_global.h
WETC_main.o: $(ARBHOME)/INCLUDE/ttypes.h
