# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .c .cpp .depend

CPP_OBJECTS = island_hopping.o i-hopper.o align.o memory.o trnsprob.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)
.c.o:
	$(ACC) $(cflags) -DARB -c $<  $(CPPINCLUDES)
.cpp.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)

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

island_hopping.o: island_hopping.h i-hopper.h defs.h memory.h
island_hopping.o: $(ARBHOME)/INCLUDE/arb_assert.h

i-hopper.o: memory.h i-hopper.h defs.h

align.o: memory.h trnsprob.h i-hopper.h defs.h

trnsprob.o: defs.h memory.h trnsprob.h
