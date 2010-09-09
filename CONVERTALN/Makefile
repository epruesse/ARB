OBJECT= \
	alma.o \
	convert.o \
	date.o \
	embl.o \
	fconv.o \
	gcg.o \
	genbank.o \
	macke.o \
	main.o \
	mg.o \
	paup.o \
	phylip.o \
	printable.o \
	routines.o \
	util.o \


.SUFFIXES: .o .cxx .depend

BINARY=$(ARBHOME)/bin/arb_convert_aln

$(BINARY): $(OBJECT)
	$(LINK_EXECUTABLE) $@ $(OBJECT) ../SL/FILE_BUFFER/FILE_BUFFER.a $(LIBPATH) $(ARBDB_LIB) $(EXECLIBS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(AINCLUDES) $(POST_COMPILE)

proto:
	../MAKEBIN/aisc_mkpt -A -w prototypes.h $(OBJECT:.o=.cxx) >prototypes.h.tmp
	../SOURCE_TOOLS/mv_if_diff prototypes.h.tmp prototypes.h

clean:
	rm -f $(OBJECT) $(BINARY)

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
alma.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
alma.o: $(ARBHOME)/INCLUDE/arb_assert.h
alma.o: $(ARBHOME)/INCLUDE/arb_core.h
alma.o: $(ARBHOME)/INCLUDE/arbdb_base.h
alma.o: $(ARBHOME)/INCLUDE/arbtools.h
alma.o: $(ARBHOME)/INCLUDE/attributes.h
alma.o: $(ARBHOME)/INCLUDE/dupstr.h
alma.o: $(ARBHOME)/INCLUDE/FileBuffer.h

convert.o: convert.h
convert.o: global.h
convert.o: prototypes.h
convert.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
convert.o: $(ARBHOME)/INCLUDE/arb_assert.h
convert.o: $(ARBHOME)/INCLUDE/arb_core.h
convert.o: $(ARBHOME)/INCLUDE/arbdb_base.h
convert.o: $(ARBHOME)/INCLUDE/arbtools.h
convert.o: $(ARBHOME)/INCLUDE/attributes.h
convert.o: $(ARBHOME)/INCLUDE/dupstr.h
convert.o: $(ARBHOME)/INCLUDE/FileBuffer.h

date.o: convert.h
date.o: global.h
date.o: prototypes.h
date.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
date.o: $(ARBHOME)/INCLUDE/arb_assert.h
date.o: $(ARBHOME)/INCLUDE/arb_core.h
date.o: $(ARBHOME)/INCLUDE/arbdb_base.h
date.o: $(ARBHOME)/INCLUDE/arbtools.h
date.o: $(ARBHOME)/INCLUDE/attributes.h
date.o: $(ARBHOME)/INCLUDE/dupstr.h
date.o: $(ARBHOME)/INCLUDE/FileBuffer.h

embl.o: convert.h
embl.o: global.h
embl.o: prototypes.h
embl.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
embl.o: $(ARBHOME)/INCLUDE/arb_assert.h
embl.o: $(ARBHOME)/INCLUDE/arb_core.h
embl.o: $(ARBHOME)/INCLUDE/arbdb_base.h
embl.o: $(ARBHOME)/INCLUDE/arbtools.h
embl.o: $(ARBHOME)/INCLUDE/attributes.h
embl.o: $(ARBHOME)/INCLUDE/dupstr.h
embl.o: $(ARBHOME)/INCLUDE/FileBuffer.h

fconv.o: convert.h
fconv.o: global.h
fconv.o: prototypes.h
fconv.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
fconv.o: $(ARBHOME)/INCLUDE/arb_assert.h
fconv.o: $(ARBHOME)/INCLUDE/arb_core.h
fconv.o: $(ARBHOME)/INCLUDE/arbdb_base.h
fconv.o: $(ARBHOME)/INCLUDE/arbtools.h
fconv.o: $(ARBHOME)/INCLUDE/attributes.h
fconv.o: $(ARBHOME)/INCLUDE/dupstr.h
fconv.o: $(ARBHOME)/INCLUDE/FileBuffer.h

gcg.o: convert.h
gcg.o: global.h
gcg.o: prototypes.h
gcg.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
gcg.o: $(ARBHOME)/INCLUDE/arb_assert.h
gcg.o: $(ARBHOME)/INCLUDE/arb_core.h
gcg.o: $(ARBHOME)/INCLUDE/arbdb_base.h
gcg.o: $(ARBHOME)/INCLUDE/arbtools.h
gcg.o: $(ARBHOME)/INCLUDE/attributes.h
gcg.o: $(ARBHOME)/INCLUDE/dupstr.h
gcg.o: $(ARBHOME)/INCLUDE/FileBuffer.h

genbank.o: convert.h
genbank.o: global.h
genbank.o: prototypes.h
genbank.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
genbank.o: $(ARBHOME)/INCLUDE/arb_assert.h
genbank.o: $(ARBHOME)/INCLUDE/arb_core.h
genbank.o: $(ARBHOME)/INCLUDE/arbdb_base.h
genbank.o: $(ARBHOME)/INCLUDE/arbtools.h
genbank.o: $(ARBHOME)/INCLUDE/attributes.h
genbank.o: $(ARBHOME)/INCLUDE/dupstr.h
genbank.o: $(ARBHOME)/INCLUDE/FileBuffer.h

macke.o: convert.h
macke.o: global.h
macke.o: prototypes.h
macke.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
macke.o: $(ARBHOME)/INCLUDE/arb_assert.h
macke.o: $(ARBHOME)/INCLUDE/arb_core.h
macke.o: $(ARBHOME)/INCLUDE/arbdb_base.h
macke.o: $(ARBHOME)/INCLUDE/arbtools.h
macke.o: $(ARBHOME)/INCLUDE/attributes.h
macke.o: $(ARBHOME)/INCLUDE/dupstr.h
macke.o: $(ARBHOME)/INCLUDE/FileBuffer.h

main.o: convert.h
main.o: global.h
main.o: prototypes.h
main.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
main.o: $(ARBHOME)/INCLUDE/arb_assert.h
main.o: $(ARBHOME)/INCLUDE/arb_core.h
main.o: $(ARBHOME)/INCLUDE/arbdb_base.h
main.o: $(ARBHOME)/INCLUDE/arbtools.h
main.o: $(ARBHOME)/INCLUDE/attributes.h
main.o: $(ARBHOME)/INCLUDE/dupstr.h
main.o: $(ARBHOME)/INCLUDE/FileBuffer.h

mg.o: convert.h
mg.o: global.h
mg.o: prototypes.h
mg.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
mg.o: $(ARBHOME)/INCLUDE/arb_assert.h
mg.o: $(ARBHOME)/INCLUDE/arb_core.h
mg.o: $(ARBHOME)/INCLUDE/arbdb_base.h
mg.o: $(ARBHOME)/INCLUDE/arbtools.h
mg.o: $(ARBHOME)/INCLUDE/attributes.h
mg.o: $(ARBHOME)/INCLUDE/dupstr.h
mg.o: $(ARBHOME)/INCLUDE/FileBuffer.h

paup.o: convert.h
paup.o: global.h
paup.o: prototypes.h
paup.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
paup.o: $(ARBHOME)/INCLUDE/arb_assert.h
paup.o: $(ARBHOME)/INCLUDE/arb_core.h
paup.o: $(ARBHOME)/INCLUDE/arbdb_base.h
paup.o: $(ARBHOME)/INCLUDE/arbtools.h
paup.o: $(ARBHOME)/INCLUDE/attributes.h
paup.o: $(ARBHOME)/INCLUDE/dupstr.h
paup.o: $(ARBHOME)/INCLUDE/FileBuffer.h

phylip.o: convert.h
phylip.o: global.h
phylip.o: prototypes.h
phylip.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
phylip.o: $(ARBHOME)/INCLUDE/arb_assert.h
phylip.o: $(ARBHOME)/INCLUDE/arb_core.h
phylip.o: $(ARBHOME)/INCLUDE/arbdb_base.h
phylip.o: $(ARBHOME)/INCLUDE/arbtools.h
phylip.o: $(ARBHOME)/INCLUDE/attributes.h
phylip.o: $(ARBHOME)/INCLUDE/dupstr.h
phylip.o: $(ARBHOME)/INCLUDE/FileBuffer.h

printable.o: convert.h
printable.o: global.h
printable.o: prototypes.h
printable.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
printable.o: $(ARBHOME)/INCLUDE/arb_assert.h
printable.o: $(ARBHOME)/INCLUDE/arb_core.h
printable.o: $(ARBHOME)/INCLUDE/arbdb_base.h
printable.o: $(ARBHOME)/INCLUDE/arbtools.h
printable.o: $(ARBHOME)/INCLUDE/attributes.h
printable.o: $(ARBHOME)/INCLUDE/dupstr.h
printable.o: $(ARBHOME)/INCLUDE/FileBuffer.h

routines.o: convert.h
routines.o: global.h
routines.o: prototypes.h
routines.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
routines.o: $(ARBHOME)/INCLUDE/arb_assert.h
routines.o: $(ARBHOME)/INCLUDE/arb_core.h
routines.o: $(ARBHOME)/INCLUDE/arbdb_base.h
routines.o: $(ARBHOME)/INCLUDE/arbtools.h
routines.o: $(ARBHOME)/INCLUDE/attributes.h
routines.o: $(ARBHOME)/INCLUDE/dupstr.h
routines.o: $(ARBHOME)/INCLUDE/FileBuffer.h

util.o: convert.h
util.o: global.h
util.o: prototypes.h
util.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
util.o: $(ARBHOME)/INCLUDE/arb_assert.h
util.o: $(ARBHOME)/INCLUDE/arb_core.h
util.o: $(ARBHOME)/INCLUDE/arbdb_base.h
util.o: $(ARBHOME)/INCLUDE/arbtools.h
util.o: $(ARBHOME)/INCLUDE/attributes.h
util.o: $(ARBHOME)/INCLUDE/dupstr.h
util.o: $(ARBHOME)/INCLUDE/FileBuffer.h
