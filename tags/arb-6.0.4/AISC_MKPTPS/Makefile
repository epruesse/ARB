# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .cxx .depend

OBJECTS = mkptypes.o
BINARY = aisc_mkpt

LOCAL_DEFINES=-DSIMPLE_ARB_ASSERT
LOCAL_MAKEDEPENDFLAGS=$(MAKEDEPENDFLAGS) $(LOCAL_DEFINES)

$(MAIN): $(BINARY)

$(BINARY): $(OBJECTS) $(use_ARB_main)
	$(LINK_EXECUTABLE) $@ $(use_ARB_main) $(OBJECTS) $(EXECLIBS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) $(LOCAL_DEFINES) -c $< $(CXX_INCLUDES) $(POST_COMPILE)

clean:
	rm -f $(OBJECTS) $(BINARY)

#depends:
#	@echo "No depends here"

DEPENDS = $(OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(LOCAL_MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.cxx.depend:
	$(MAKEDEPEND) -f- $(LOCAL_MAKEDEPENDFLAGS) $< 2>/dev/null >$@


# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

mkptypes.o: $(ARBHOME)/INCLUDE/arb_assert.h
mkptypes.o: $(ARBHOME)/INCLUDE/arb_early_check.h
mkptypes.o: $(ARBHOME)/INCLUDE/arb_simple_assert.h
mkptypes.o: $(ARBHOME)/INCLUDE/arbtools.h
mkptypes.o: $(ARBHOME)/INCLUDE/attributes.h
mkptypes.o: $(ARBHOME)/INCLUDE/cxxforward.h
mkptypes.o: $(ARBHOME)/INCLUDE/dupstr.h
mkptypes.o: $(ARBHOME)/INCLUDE/gccver.h
mkptypes.o: $(ARBHOME)/INCLUDE/static_assert.h
mkptypes.o: $(ARBHOME)/INCLUDE/test_global.h
mkptypes.o: $(ARBHOME)/INCLUDE/test_unit.h
