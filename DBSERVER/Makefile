# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .c .cxx .depend
OBJECTS = db_server.o

$(MAIN): $(OBJECTS)
	$(AR) $(MAIN) $(OBJECTS)
$(OBJECTS): $(LOCALS) $(ARBDBH) $(SERVERCNTRLH)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

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

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

db_server.o: $(ARBHOME)/INCLUDE/ad_prot.h
db_server.o: $(ARBHOME)/INCLUDE/arb_assert.h
db_server.o: $(ARBHOME)/INCLUDE/arbdb.h
db_server.o: $(ARBHOME)/INCLUDE/servercntrl.h
