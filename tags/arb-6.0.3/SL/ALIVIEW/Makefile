# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = AliView.o
OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE)

.c.o:
	$(A_CC) $(cflags) -c $< $(CC_INCLUDES) $(POST_COMPILE)

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

AliView.o: AliView.hxx
AliView.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
AliView.o: $(ARBHOME)/INCLUDE/arb_assert.h
AliView.o: $(ARBHOME)/INCLUDE/arb_core.h
AliView.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AliView.o: $(ARBHOME)/INCLUDE/arbtools.h
AliView.o: $(ARBHOME)/INCLUDE/attributes.h
AliView.o: $(ARBHOME)/INCLUDE/cxxforward.h
AliView.o: $(ARBHOME)/INCLUDE/dupstr.h
AliView.o: $(ARBHOME)/INCLUDE/gccver.h
AliView.o: $(ARBHOME)/INCLUDE/test_global.h
