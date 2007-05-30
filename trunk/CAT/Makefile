# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

CPP_OBJECTS =   CAT_tree.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

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
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

CAT_tree.o: cat_tree.hxx
CAT_tree.o: $(ARBHOME)/INCLUDE/ad_prot.h
CAT_tree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
CAT_tree.o: $(ARBHOME)/INCLUDE/arb_assert.h
CAT_tree.o: $(ARBHOME)/INCLUDE/arbdb.h
CAT_tree.o: $(ARBHOME)/INCLUDE/arbdbt.h
