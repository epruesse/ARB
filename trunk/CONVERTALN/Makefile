OBJECT=  main.o fconv.o mg.o genbank.o macke.o phylip.o paup.o \
	util.o date.o embl.o gcg.o printable.o alma.o \
	routines.o

.SUFFIXES: .o .c .cxx

$(ARBHOME)/bin/arb_convert_aln: $(OBJECT)
	$(ACC) $(cflags) -o $@ $(OBJECT)
.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES)

proto:
	rm -f prototypes.h
	../MAKEBIN/aisc_mkpt -C $(OBJECT:.o=.c) >prototypes.h

depend:
	$(MAKEDEPEND) $(OBJECT:.o=.c*)

# DO NOT DELETE THIS LINE -- make depend depends on it.

main.o:  
main.o:  
main.o:  
main.o:  convert.h global.h prototypes.h
fconv.o:  
fconv.o:  
fconv.o:   convert.h
fconv.o: global.h prototypes.h
mg.o:  
mg.o:  
mg.o:   convert.h global.h
mg.o: prototypes.h
genbank.o:  
genbank.o:  
genbank.o:  
genbank.o:  convert.h global.h prototypes.h
macke.o:  
macke.o:  
macke.o:   convert.h
macke.o: global.h prototypes.h
phylip.o:  
phylip.o:  
phylip.o:   convert.h
phylip.o: global.h prototypes.h
paup.o:  
paup.o:  
paup.o:   convert.h global.h
paup.o: prototypes.h
util.o:  
util.o:  
util.o:  
util.o:   
util.o: convert.h global.h prototypes.h
date.o:  
date.o:  
date.o:  
date.o:   
date.o:  
date.o:  
date.o:  convert.h global.h prototypes.h
embl.o:  
embl.o:  
embl.o:   convert.h global.h
embl.o: prototypes.h
gcg.o:  
gcg.o:  
gcg.o:   convert.h global.h
gcg.o: prototypes.h
printable.o:  
printable.o:  
printable.o:   convert.h
printable.o: global.h prototypes.h
alma.o:  
alma.o:  
alma.o:  
alma.o:  convert.h global.h prototypes.h
routines.o:  
routines.o:  
routines.o:   convert.h
routines.o: global.h prototypes.h
