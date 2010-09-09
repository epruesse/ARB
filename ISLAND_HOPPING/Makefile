# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .c .cxx .depend

CPP_OBJECTS = island_hopping.o i-hopper.o align.o mem.o trnsprob.o

$(MAIN): $(CPP_OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)
.c.o:
	$(ACC) $(cflags) -DARB -c $<  $(CPPINCLUDES)
.cxx.o:
	$(CPP) $(cflags) -DARB -c $<  $(CPPINCLUDES)

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

align.o: defs.h
align.o: i-hopper.h
align.o: mem.h
align.o: trnsprob.h

i-hopper.o: defs.h
i-hopper.o: i-hopper.h
i-hopper.o: mem.h

island_hopping.o: defs.h
island_hopping.o: i-hopper.h
island_hopping.o: island_hopping.h
island_hopping.o: mem.h
island_hopping.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
island_hopping.o: $(ARBHOME)/INCLUDE/arb_assert.h
island_hopping.o: $(ARBHOME)/INCLUDE/arb_core.h
island_hopping.o: $(ARBHOME)/INCLUDE/arbdb_base.h
island_hopping.o: $(ARBHOME)/INCLUDE/attributes.h
island_hopping.o: $(ARBHOME)/INCLUDE/dupstr.h
island_hopping.o: $(ARBHOME)/INCLUDE/test_global.h

mem.o: mem.h

trnsprob.o: defs.h
trnsprob.o: mem.h
trnsprob.o: trnsprob.h
trnsprob.o: $(ARBHOME)/INCLUDE/arb_assert.h
trnsprob.o: $(ARBHOME)/INCLUDE/test_global.h
