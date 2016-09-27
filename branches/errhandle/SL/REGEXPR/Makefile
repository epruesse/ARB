# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = RegExpr.o
OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

.c.o:
	$(A_CC) $(cflags) -c $< $(CC_INCLUDES) $(POST_COMPILE) $<

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

RegExpr.o: RegExpr.hxx
RegExpr.o: $(ARBHOME)/INCLUDE/arb_assert.h
RegExpr.o: $(ARBHOME)/INCLUDE/arb_core.h
RegExpr.o: $(ARBHOME)/INCLUDE/arb_match.h
RegExpr.o: $(ARBHOME)/INCLUDE/arb_mem.h
RegExpr.o: $(ARBHOME)/INCLUDE/arbtools.h
RegExpr.o: $(ARBHOME)/INCLUDE/attributes.h
RegExpr.o: $(ARBHOME)/INCLUDE/cxxforward.h
RegExpr.o: $(ARBHOME)/INCLUDE/dupstr.h
RegExpr.o: $(ARBHOME)/INCLUDE/gccver.h
RegExpr.o: $(ARBHOME)/INCLUDE/test_global.h
RegExpr.o: $(ARBHOME)/INCLUDE/test_unit.h
