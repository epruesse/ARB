# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .c .cxx .o .h .depend

CPP_OBJECTS =
C_OBJECTS = base.o defines.o frand.o readcfg.o rns.o sim.o simcfg.o spreadin.o
OBJECTS = $(CPP_OBJECTS) $(C_OBJECTS)

$(MAIN): $(CPP_OBJECTS) $(C_OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS) $(C_OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE)

.c.o:
	$(A_CC) $(cflags) -c $< $(CC_INCLUDES) $(POST_COMPILE)

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

base.o: base.h
base.o: defines.h
base.o: $(ARBHOME)/INCLUDE/attributes.h
base.o: $(ARBHOME)/INCLUDE/gccver.h

defines.o: defines.h
defines.o: $(ARBHOME)/INCLUDE/attributes.h
defines.o: $(ARBHOME)/INCLUDE/gccver.h

frand.o: defines.h
frand.o: frand.h
frand.o: $(ARBHOME)/INCLUDE/attributes.h
frand.o: $(ARBHOME)/INCLUDE/gccver.h

readcfg.o: defines.h
readcfg.o: readcfg.h
readcfg.o: $(ARBHOME)/INCLUDE/attributes.h
readcfg.o: $(ARBHOME)/INCLUDE/gccver.h

rns.o: base.h
rns.o: defines.h
rns.o: frand.h
rns.o: rns.h
rns.o: simcfg.h
rns.o: spreadin.h
rns.o: $(ARBHOME)/INCLUDE/attributes.h
rns.o: $(ARBHOME)/INCLUDE/gccver.h

sim.o: base.h
sim.o: defines.h
sim.o: frand.h
sim.o: rns.h
sim.o: simcfg.h
sim.o: $(ARBHOME)/INCLUDE/attributes.h
sim.o: $(ARBHOME)/INCLUDE/gccver.h

simcfg.o: base.h
simcfg.o: defines.h
simcfg.o: frand.h
simcfg.o: readcfg.h
simcfg.o: rns.h
simcfg.o: simcfg.h
simcfg.o: $(ARBHOME)/INCLUDE/attributes.h
simcfg.o: $(ARBHOME)/INCLUDE/gccver.h

spreadin.o: defines.h
spreadin.o: spreadin.h
spreadin.o: $(ARBHOME)/INCLUDE/attributes.h
spreadin.o: $(ARBHOME)/INCLUDE/gccver.h
