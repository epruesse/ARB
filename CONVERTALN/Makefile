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

main.o: convert.h global.h prototypes.h

fconv.o: convert.h global.h prototypes.h

mg.o: convert.h global.h prototypes.h

genbank.o: convert.h global.h prototypes.h

macke.o: convert.h global.h prototypes.h

phylip.o: convert.h global.h prototypes.h

paup.o: convert.h global.h prototypes.h

util.o: convert.h global.h prototypes.h

date.o: convert.h global.h prototypes.h

embl.o: convert.h global.h prototypes.h

gcg.o: convert.h global.h prototypes.h

printable.o: convert.h global.h prototypes.h

alma.o: convert.h global.h prototypes.h

routines.o: convert.h global.h prototypes.h
