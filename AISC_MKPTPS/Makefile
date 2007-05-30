# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .depend

OBJECTS = mkptypes.o
BINARY = aisc_mkpt

$(MAIN): $(BINARY)

$(BINARY): $(OBJECTS)
	$(ACC) $(cflags) -o $@ $(OBJECTS)

.c.o:
	$(ACC) $(cflags) -c $<

clean:
	rm -f $(OBJECTS) $(BINARY)

DEPENDS = $(OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

mkptypes.o: patchlev.h
