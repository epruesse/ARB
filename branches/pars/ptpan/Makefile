.SUFFIXES: .o .cxx .depend
OBJECTS = $(subst .cxx,.o,$(wildcard *.cxx)) 

# --------------------------------------------------------------------------------

$(MAIN): $(OBJECTS) Makefile
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS) 

$(OBJECTS): Makefile

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $<  $(CXX_INCLUDES) $(POST_COMPILE) $<

proto:
	@echo Nothing to do for proto here  

clean:
	rm -f $(OBJECTS) *.a

DEPENDS = $(OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
	-@rm Makefile.bak # get rid of bak (generated for some dubious reason only in this directory)
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

PTP_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
PTP_main.o: $(ARBHOME)/INCLUDE/arb_core.h
PTP_main.o: $(ARBHOME)/INCLUDE/dupstr.h
PTP_main.o: $(ARBHOME)/INCLUDE/test_global.h
