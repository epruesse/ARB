.SUFFIXES: .o .c .depend

OBJECTS = clustalv.o amenu.o myers.o sequence.o showpair.o \
          upgma.o util.o trees.o gcgcheck.o


$(ARBHOME)/bin/clustalv : $(OBJECTS)
	$(ACC) $(cflags) -o $@ $(OBJECTS) -lm

.c.o:
	$(ACC) $(cflags) -c -o $@ $< $(AINCLUDES)

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

clustalv.o: clustalv.h general.h matrices.h

amenu.o: clustalv.h general.h

myers.o: clustalv.h general.h

sequence.o: clustalv.h general.h

showpair.o: clustalv.h general.h

upgma.o: clustalv.h general.h

util.o: clustalv.h general.h

trees.o: clustalv.h general.h
