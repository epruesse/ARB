# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend


CPP_OBJECTS = xml.o

$(MAIN): $(CPP_OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

xml.o: xml.hxx
xml.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
xml.o: $(ARBHOME)/INCLUDE/arb_assert.h
xml.o: $(ARBHOME)/INCLUDE/arb_core.h
xml.o: $(ARBHOME)/INCLUDE/arbdb_base.h
xml.o: $(ARBHOME)/INCLUDE/attributes.h
xml.o: $(ARBHOME)/INCLUDE/dupstr.h
xml.o: $(ARBHOME)/INCLUDE/test_global.h
