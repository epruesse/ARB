# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .c .depend
OBJECTS = names.o
$(MAIN): $(OBJECTS)
	$(AR) $(MAIN) $(OBJECTS)
.c.o:
	$(ACC) $(cflags) -c $<  $(AINCLUDES)
.cxx.o:
	$(CPP) $(cflags) -I/opt/gcc.3.4.3/include/c++/3.4.3/include -c $<  $(CPPINCLUDES)

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

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

names.o: names.h
names.o: $(ARBHOME)/INCLUDE/ad_prot.h
names.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
names.o: $(ARBHOME)/INCLUDE/arb_assert.h
names.o: $(ARBHOME)/INCLUDE/arbdb.h
names.o: $(ARBHOME)/INCLUDE/client.h
names.o: $(ARBHOME)/INCLUDE/server.h
names.o: $(ARBHOME)/INCLUDE/servercntrl.h
names.o: $(ARBHOME)/INCLUDE/struct_man.h
