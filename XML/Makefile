# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .cxx .depend


CPP_OBJECTS = xml.o

$(MAIN): $(CPP_OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE)

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

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

xml.o: xml.hxx
xml.o: $(ARBHOME)/INCLUDE/arb_assert.h
xml.o: $(ARBHOME)/INCLUDE/arb_core.h
xml.o: $(ARBHOME)/INCLUDE/arbtools.h
xml.o: $(ARBHOME)/INCLUDE/attributes.h
xml.o: $(ARBHOME)/INCLUDE/cxxforward.h
xml.o: $(ARBHOME)/INCLUDE/dupstr.h
xml.o: $(ARBHOME)/INCLUDE/gccver.h
xml.o: $(ARBHOME)/INCLUDE/test_global.h
