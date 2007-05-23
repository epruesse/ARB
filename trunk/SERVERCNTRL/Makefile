# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .c .cxx .depend
OBJECTS = servercntrl.o
$(MAIN): $(OBJECTS)
	$(AR) $(@) $(OBJECTS)
.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES)
.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

proto:
	../MAKEBIN/aisc_mkpt -A -P -C $(OBJECTS:.o=.c) >servercntrl.h.tmp
	../SOURCE_TOOLS/mv_if_diff servercntrl.h.tmp servercntrl.h

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

servercntrl.o: servercntrl.h
servercntrl.o: $(ARBHOME)/INCLUDE/ad_prot.h
servercntrl.o: $(ARBHOME)/INCLUDE/aisc_global.h
servercntrl.o: $(ARBHOME)/INCLUDE/arb_assert.h
servercntrl.o: $(ARBHOME)/INCLUDE/arbdb.h
servercntrl.o: $(ARBHOME)/INCLUDE/client.h
servercntrl.o: $(ARBHOME)/INCLUDE/client_privat.h
