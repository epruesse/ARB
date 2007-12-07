.SUFFIXES: .o .c .depend

OBJECTS = clustalv.o amenu.o myers.o sequence.o showpair.o \
          upgma.o util.o trees.o gcgcheck.o

BINARY=$(ARBHOME)/bin/clustalv

$(BINARY) : $(OBJECTS)
	$(ACC) $(cflags) -o $@ $(OBJECTS) -lm

.c.o:
	$(ACC) $(cflags) -c -o $@ $< $(AINCLUDES)

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

amenu.o: clustalv.h
amenu.o: general.h

clustalv.o: clustalv.h
clustalv.o: general.h
clustalv.o: matrices.h

myers.o: clustalv.h
myers.o: general.h

sequence.o: clustalv.h
sequence.o: general.h

showpair.o: clustalv.h
showpair.o: general.h

trees.o: clustalv.h
trees.o: general.h

upgma.o: clustalv.h
upgma.o: general.h

util.o: clustalv.h
util.o: general.h
