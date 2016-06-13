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

item_shader.o: item_shader.h
item_shader.o: $(ARBHOME)/INCLUDE/arb_assert.h
item_shader.o: $(ARBHOME)/INCLUDE/arb_core.h
item_shader.o: $(ARBHOME)/INCLUDE/arbtools.h
item_shader.o: $(ARBHOME)/INCLUDE/attributes.h
item_shader.o: $(ARBHOME)/INCLUDE/cxxforward.h
item_shader.o: $(ARBHOME)/INCLUDE/dupstr.h
item_shader.o: $(ARBHOME)/INCLUDE/gccver.h
item_shader.o: $(ARBHOME)/INCLUDE/smartptr.h
item_shader.o: $(ARBHOME)/INCLUDE/test_global.h
item_shader.o: $(ARBHOME)/INCLUDE/test_unit.h

ValueTuple.o: item_shader.h
ValueTuple.o: $(ARBHOME)/INCLUDE/arb_assert.h
ValueTuple.o: $(ARBHOME)/INCLUDE/arb_core.h
ValueTuple.o: $(ARBHOME)/INCLUDE/arbtools.h
ValueTuple.o: $(ARBHOME)/INCLUDE/attributes.h
ValueTuple.o: $(ARBHOME)/INCLUDE/cxxforward.h
ValueTuple.o: $(ARBHOME)/INCLUDE/dupstr.h
ValueTuple.o: $(ARBHOME)/INCLUDE/gccver.h
ValueTuple.o: $(ARBHOME)/INCLUDE/smartptr.h
ValueTuple.o: $(ARBHOME)/INCLUDE/test_global.h
ValueTuple.o: $(ARBHOME)/INCLUDE/test_unit.h
