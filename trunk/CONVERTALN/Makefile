OBJECT=  main.o fconv.o mg.o genbank.o macke.o phylip.o paup.o \
	util.o date.o embl.o gcg.o printable.o alma.o \
	routines.o

.SUFFIXES: .o .c .cxx .depend

$(ARBHOME)/bin/arb_convert_aln: $(OBJECT)
	$(ACC) $(cflags) -o $@ $(OBJECT)
.c.o:
	$(ACC) $(cflags) -Wstrict-prototypes -c $< $(AINCLUDES)

proto:
	rm -f prototypes.h
	../MAKEBIN/aisc_mkpt -C $(OBJECT:.o=.c) >prototypes.h

#depend:
#	$(MAKEDEPEND) $(OBJECT:.o=.c*)

DEPENDS = $(OBJECT:.o=.depend)
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

alma.o: convert.h
alma.o: global.h
alma.o: prototypes.h

date.o: convert.h
date.o: global.h
date.o: prototypes.h

embl.o: convert.h
embl.o: global.h
embl.o: prototypes.h

fconv.o: convert.h
fconv.o: global.h
fconv.o: prototypes.h

gcg.o: convert.h
gcg.o: global.h
gcg.o: prototypes.h

genbank.o: convert.h
genbank.o: global.h
genbank.o: prototypes.h

macke.o: convert.h
macke.o: global.h
macke.o: prototypes.h

main.o: convert.h
main.o: global.h
main.o: prototypes.h

mg.o: convert.h
mg.o: global.h
mg.o: prototypes.h

paup.o: convert.h
paup.o: global.h
paup.o: prototypes.h

phylip.o: convert.h
phylip.o: global.h
phylip.o: prototypes.h

printable.o: convert.h
printable.o: global.h
printable.o: prototypes.h

routines.o: convert.h
routines.o: global.h
routines.o: prototypes.h

util.o: convert.h
util.o: global.h
util.o: prototypes.h
