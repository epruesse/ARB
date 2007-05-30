# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .c .cxx .o .h .depend

CPP_OBJECTS =
C_OBJECTS = base.o defines.o frand.o readcfg.o rns.o sim.o simcfg.o spreadin.o
OBJECTS = $(CPP_OBJECTS) $(C_OBJECTS)

$(MAIN): $(CPP_OBJECTS) $(C_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS) $(C_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

.c.o:
	$(ACC) $(cflags) -c $< $(CPPINCLUDES)

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

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

base.o: base.h
base.o: defines.h

defines.o: defines.h

frand.o: defines.h
frand.o: frand.h

readcfg.o: defines.h
readcfg.o: readcfg.h

rns.o: base.h
rns.o: defines.h
rns.o: frand.h
rns.o: rns.h
rns.o: simcfg.h
rns.o: spreadin.h

sim.o: base.h
sim.o: defines.h
sim.o: frand.h
sim.o: rns.h
sim.o: simcfg.h

simcfg.o: base.h
simcfg.o: defines.h
simcfg.o: frand.h
simcfg.o: readcfg.h
simcfg.o: rns.h
simcfg.o: simcfg.h

spreadin.o: defines.h
spreadin.o: spreadin.h
