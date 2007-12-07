# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .c .cpp .depend

CPP_OBJECTS = island_hopping.o i-hopper.o align.o memory.o trnsprob.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)
.c.o:
	$(ACC) $(cflags) -DARB -c $<  $(CPPINCLUDES)
.cpp.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)

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
.cpp.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

align.o: defs.h
align.o: i-hopper.h
align.o: memory.h
align.o: trnsprob.h

i-hopper.o: defs.h
i-hopper.o: i-hopper.h
i-hopper.o: memory.h

island_hopping.o: defs.h
island_hopping.o: i-hopper.h
island_hopping.o: island_hopping.h
island_hopping.o: memory.h
island_hopping.o: $(ARBHOME)/INCLUDE/arb_assert.h

trnsprob.o: defs.h
trnsprob.o: memory.h
trnsprob.o: trnsprob.h
