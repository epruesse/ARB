.SUFFIXES: .o .a .cxx .depend .stamp

SOURCES=$(wildcard *.cxx)
HEADERS=$(wildcard *.h)
OBJECTS= $(subst .cxx,.o,$(SOURCES))
ERROR_CHECKER=./show_error_list.pl

$(MAIN): $(OBJECTS) check_error_codes.stamp
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

check_error_codes.stamp: $(ERROR_CHECKER) $(SOURCES) $(HEADERS)
	@$(ERROR_CHECKER) -check
	@touch $@

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

proto:
	../AISC_MKPTPS/aisc_mkpt -P -G -w prototypes.h $(OBJECTS:.o=.cxx) >prototypes.h.tmp
	../SOURCE_TOOLS/mv_if_diff prototypes.h.tmp prototypes.h

clean:
	rm -f *.o $(BINARY) *.stamp

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

convert.o: defs.h
convert.o: embl.h
convert.o: fun.h
convert.o: genbank.h
convert.o: global.h
convert.o: input_format.h
convert.o: macke.h
convert.o: parser.h
convert.o: prototypes.h
convert.o: rdp_info.h
convert.o: reader.h
convert.o: refs.h
convert.o: seq.h
convert.o: $(ARBHOME)/INCLUDE/arb_assert.h
convert.o: $(ARBHOME)/INCLUDE/arb_core.h
convert.o: $(ARBHOME)/INCLUDE/arb_defs.h
convert.o: $(ARBHOME)/INCLUDE/arbtools.h
convert.o: $(ARBHOME)/INCLUDE/attributes.h
convert.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
convert.o: $(ARBHOME)/INCLUDE/cxxforward.h
convert.o: $(ARBHOME)/INCLUDE/dupstr.h
convert.o: $(ARBHOME)/INCLUDE/gccver.h
convert.o: $(ARBHOME)/INCLUDE/smartptr.h
convert.o: $(ARBHOME)/INCLUDE/test_global.h

date.o: fun.h
date.o: global.h
date.o: prototypes.h
date.o: $(ARBHOME)/INCLUDE/arb_assert.h
date.o: $(ARBHOME)/INCLUDE/arb_core.h
date.o: $(ARBHOME)/INCLUDE/arb_defs.h
date.o: $(ARBHOME)/INCLUDE/arbtools.h
date.o: $(ARBHOME)/INCLUDE/attributes.h
date.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
date.o: $(ARBHOME)/INCLUDE/cxxforward.h
date.o: $(ARBHOME)/INCLUDE/dupstr.h
date.o: $(ARBHOME)/INCLUDE/gccver.h
date.o: $(ARBHOME)/INCLUDE/smartptr.h
date.o: $(ARBHOME)/INCLUDE/test_global.h
date.o: $(ARBHOME)/INCLUDE/test_unit.h

embl.o: defs.h
embl.o: embl.h
embl.o: fun.h
embl.o: genbank.h
embl.o: global.h
embl.o: input_format.h
embl.o: macke.h
embl.o: parser.h
embl.o: prototypes.h
embl.o: rdp_info.h
embl.o: reader.h
embl.o: refs.h
embl.o: seq.h
embl.o: wrap.h
embl.o: $(ARBHOME)/INCLUDE/arb_assert.h
embl.o: $(ARBHOME)/INCLUDE/arb_core.h
embl.o: $(ARBHOME)/INCLUDE/arb_defs.h
embl.o: $(ARBHOME)/INCLUDE/arbtools.h
embl.o: $(ARBHOME)/INCLUDE/attributes.h
embl.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
embl.o: $(ARBHOME)/INCLUDE/cxxforward.h
embl.o: $(ARBHOME)/INCLUDE/dupstr.h
embl.o: $(ARBHOME)/INCLUDE/gccver.h
embl.o: $(ARBHOME)/INCLUDE/smartptr.h
embl.o: $(ARBHOME)/INCLUDE/test_global.h
embl.o: $(ARBHOME)/INCLUDE/test_unit.h

fconv.o: defs.h
fconv.o: fun.h
fconv.o: global.h
fconv.o: prototypes.h
fconv.o: $(ARBHOME)/INCLUDE/ad_prot.h
fconv.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
fconv.o: $(ARBHOME)/INCLUDE/arb_assert.h
fconv.o: $(ARBHOME)/INCLUDE/arb_core.h
fconv.o: $(ARBHOME)/INCLUDE/arb_defs.h
fconv.o: $(ARBHOME)/INCLUDE/arb_diff.h
fconv.o: $(ARBHOME)/INCLUDE/arb_error.h
fconv.o: $(ARBHOME)/INCLUDE/arb_file.h
fconv.o: $(ARBHOME)/INCLUDE/arb_mem.h
fconv.o: $(ARBHOME)/INCLUDE/arb_msg.h
fconv.o: $(ARBHOME)/INCLUDE/arb_string.h
fconv.o: $(ARBHOME)/INCLUDE/arbdb.h
fconv.o: $(ARBHOME)/INCLUDE/arbdb_base.h
fconv.o: $(ARBHOME)/INCLUDE/arbdbt.h
fconv.o: $(ARBHOME)/INCLUDE/arbtools.h
fconv.o: $(ARBHOME)/INCLUDE/attributes.h
fconv.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
fconv.o: $(ARBHOME)/INCLUDE/cxxforward.h
fconv.o: $(ARBHOME)/INCLUDE/downcast.h
fconv.o: $(ARBHOME)/INCLUDE/dupstr.h
fconv.o: $(ARBHOME)/INCLUDE/gccver.h
fconv.o: $(ARBHOME)/INCLUDE/smartptr.h
fconv.o: $(ARBHOME)/INCLUDE/static_assert.h
fconv.o: $(ARBHOME)/INCLUDE/test_global.h
fconv.o: $(ARBHOME)/INCLUDE/test_unit.h

gcg.o: defs.h
gcg.o: embl.h
gcg.o: fun.h
gcg.o: genbank.h
gcg.o: global.h
gcg.o: input_format.h
gcg.o: macke.h
gcg.o: parser.h
gcg.o: prototypes.h
gcg.o: rdp_info.h
gcg.o: reader.h
gcg.o: refs.h
gcg.o: seq.h
gcg.o: $(ARBHOME)/INCLUDE/arb_assert.h
gcg.o: $(ARBHOME)/INCLUDE/arb_core.h
gcg.o: $(ARBHOME)/INCLUDE/arb_defs.h
gcg.o: $(ARBHOME)/INCLUDE/arbtools.h
gcg.o: $(ARBHOME)/INCLUDE/attributes.h
gcg.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
gcg.o: $(ARBHOME)/INCLUDE/cxxforward.h
gcg.o: $(ARBHOME)/INCLUDE/dupstr.h
gcg.o: $(ARBHOME)/INCLUDE/gccver.h
gcg.o: $(ARBHOME)/INCLUDE/smartptr.h
gcg.o: $(ARBHOME)/INCLUDE/test_global.h

genbank.o: defs.h
genbank.o: fun.h
genbank.o: genbank.h
genbank.o: global.h
genbank.o: input_format.h
genbank.o: parser.h
genbank.o: prototypes.h
genbank.o: rdp_info.h
genbank.o: reader.h
genbank.o: refs.h
genbank.o: seq.h
genbank.o: wrap.h
genbank.o: $(ARBHOME)/INCLUDE/arb_assert.h
genbank.o: $(ARBHOME)/INCLUDE/arb_core.h
genbank.o: $(ARBHOME)/INCLUDE/arb_defs.h
genbank.o: $(ARBHOME)/INCLUDE/arbtools.h
genbank.o: $(ARBHOME)/INCLUDE/attributes.h
genbank.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
genbank.o: $(ARBHOME)/INCLUDE/cxxforward.h
genbank.o: $(ARBHOME)/INCLUDE/dupstr.h
genbank.o: $(ARBHOME)/INCLUDE/gccver.h
genbank.o: $(ARBHOME)/INCLUDE/smartptr.h
genbank.o: $(ARBHOME)/INCLUDE/test_global.h

macke.o: defs.h
macke.o: fun.h
macke.o: global.h
macke.o: input_format.h
macke.o: macke.h
macke.o: parser.h
macke.o: prototypes.h
macke.o: rdp_info.h
macke.o: reader.h
macke.o: seq.h
macke.o: wrap.h
macke.o: $(ARBHOME)/INCLUDE/arb_assert.h
macke.o: $(ARBHOME)/INCLUDE/arb_core.h
macke.o: $(ARBHOME)/INCLUDE/arb_defs.h
macke.o: $(ARBHOME)/INCLUDE/arbtools.h
macke.o: $(ARBHOME)/INCLUDE/attributes.h
macke.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
macke.o: $(ARBHOME)/INCLUDE/cxxforward.h
macke.o: $(ARBHOME)/INCLUDE/dupstr.h
macke.o: $(ARBHOME)/INCLUDE/gccver.h
macke.o: $(ARBHOME)/INCLUDE/smartptr.h
macke.o: $(ARBHOME)/INCLUDE/test_global.h

main.o: defs.h
main.o: fun.h
main.o: global.h
main.o: prototypes.h
main.o: $(ARBHOME)/INCLUDE/arb_assert.h
main.o: $(ARBHOME)/INCLUDE/arb_core.h
main.o: $(ARBHOME)/INCLUDE/arb_defs.h
main.o: $(ARBHOME)/INCLUDE/arb_msg.h
main.o: $(ARBHOME)/INCLUDE/arbtools.h
main.o: $(ARBHOME)/INCLUDE/attributes.h
main.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
main.o: $(ARBHOME)/INCLUDE/cxxforward.h
main.o: $(ARBHOME)/INCLUDE/dupstr.h
main.o: $(ARBHOME)/INCLUDE/gccver.h
main.o: $(ARBHOME)/INCLUDE/smartptr.h
main.o: $(ARBHOME)/INCLUDE/test_global.h
main.o: $(ARBHOME)/INCLUDE/test_unit.h

mg.o: defs.h
mg.o: fun.h
mg.o: genbank.h
mg.o: global.h
mg.o: input_format.h
mg.o: macke.h
mg.o: parser.h
mg.o: prototypes.h
mg.o: rdp_info.h
mg.o: reader.h
mg.o: refs.h
mg.o: seq.h
mg.o: $(ARBHOME)/INCLUDE/arb_assert.h
mg.o: $(ARBHOME)/INCLUDE/arb_core.h
mg.o: $(ARBHOME)/INCLUDE/arb_defs.h
mg.o: $(ARBHOME)/INCLUDE/arbtools.h
mg.o: $(ARBHOME)/INCLUDE/attributes.h
mg.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
mg.o: $(ARBHOME)/INCLUDE/cxxforward.h
mg.o: $(ARBHOME)/INCLUDE/dupstr.h
mg.o: $(ARBHOME)/INCLUDE/gccver.h
mg.o: $(ARBHOME)/INCLUDE/smartptr.h
mg.o: $(ARBHOME)/INCLUDE/test_global.h

paup.o: ali.h
paup.o: defs.h
paup.o: fun.h
paup.o: global.h
paup.o: input_format.h
paup.o: paup.h
paup.o: prototypes.h
paup.o: reader.h
paup.o: seq.h
paup.o: $(ARBHOME)/INCLUDE/arb_assert.h
paup.o: $(ARBHOME)/INCLUDE/arb_core.h
paup.o: $(ARBHOME)/INCLUDE/arb_defs.h
paup.o: $(ARBHOME)/INCLUDE/arbtools.h
paup.o: $(ARBHOME)/INCLUDE/attributes.h
paup.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
paup.o: $(ARBHOME)/INCLUDE/cxxforward.h
paup.o: $(ARBHOME)/INCLUDE/dupstr.h
paup.o: $(ARBHOME)/INCLUDE/gccver.h
paup.o: $(ARBHOME)/INCLUDE/smartptr.h
paup.o: $(ARBHOME)/INCLUDE/test_global.h

phylip.o: ali.h
phylip.o: defs.h
phylip.o: fun.h
phylip.o: global.h
phylip.o: input_format.h
phylip.o: prototypes.h
phylip.o: reader.h
phylip.o: seq.h
phylip.o: $(ARBHOME)/INCLUDE/arb_assert.h
phylip.o: $(ARBHOME)/INCLUDE/arb_core.h
phylip.o: $(ARBHOME)/INCLUDE/arb_defs.h
phylip.o: $(ARBHOME)/INCLUDE/arbtools.h
phylip.o: $(ARBHOME)/INCLUDE/attributes.h
phylip.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
phylip.o: $(ARBHOME)/INCLUDE/cxxforward.h
phylip.o: $(ARBHOME)/INCLUDE/dupstr.h
phylip.o: $(ARBHOME)/INCLUDE/gccver.h
phylip.o: $(ARBHOME)/INCLUDE/smartptr.h
phylip.o: $(ARBHOME)/INCLUDE/test_global.h

printable.o: ali.h
printable.o: defs.h
printable.o: fun.h
printable.o: global.h
printable.o: input_format.h
printable.o: prototypes.h
printable.o: reader.h
printable.o: seq.h
printable.o: $(ARBHOME)/INCLUDE/arb_assert.h
printable.o: $(ARBHOME)/INCLUDE/arb_core.h
printable.o: $(ARBHOME)/INCLUDE/arb_defs.h
printable.o: $(ARBHOME)/INCLUDE/arbtools.h
printable.o: $(ARBHOME)/INCLUDE/attributes.h
printable.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
printable.o: $(ARBHOME)/INCLUDE/cxxforward.h
printable.o: $(ARBHOME)/INCLUDE/dupstr.h
printable.o: $(ARBHOME)/INCLUDE/gccver.h
printable.o: $(ARBHOME)/INCLUDE/smartptr.h
printable.o: $(ARBHOME)/INCLUDE/test_global.h

rdp_info.o: defs.h
rdp_info.o: fun.h
rdp_info.o: global.h
rdp_info.o: input_format.h
rdp_info.o: prototypes.h
rdp_info.o: rdp_info.h
rdp_info.o: reader.h
rdp_info.o: seq.h
rdp_info.o: $(ARBHOME)/INCLUDE/arb_assert.h
rdp_info.o: $(ARBHOME)/INCLUDE/arb_core.h
rdp_info.o: $(ARBHOME)/INCLUDE/arb_defs.h
rdp_info.o: $(ARBHOME)/INCLUDE/arbtools.h
rdp_info.o: $(ARBHOME)/INCLUDE/attributes.h
rdp_info.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
rdp_info.o: $(ARBHOME)/INCLUDE/cxxforward.h
rdp_info.o: $(ARBHOME)/INCLUDE/dupstr.h
rdp_info.o: $(ARBHOME)/INCLUDE/gccver.h
rdp_info.o: $(ARBHOME)/INCLUDE/smartptr.h
rdp_info.o: $(ARBHOME)/INCLUDE/test_global.h

reader.o: defs.h
reader.o: fun.h
reader.o: global.h
reader.o: input_format.h
reader.o: prototypes.h
reader.o: reader.h
reader.o: seq.h
reader.o: $(ARBHOME)/INCLUDE/arb_assert.h
reader.o: $(ARBHOME)/INCLUDE/arb_core.h
reader.o: $(ARBHOME)/INCLUDE/arb_defs.h
reader.o: $(ARBHOME)/INCLUDE/arbtools.h
reader.o: $(ARBHOME)/INCLUDE/attributes.h
reader.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
reader.o: $(ARBHOME)/INCLUDE/cxxforward.h
reader.o: $(ARBHOME)/INCLUDE/dupstr.h
reader.o: $(ARBHOME)/INCLUDE/gccver.h
reader.o: $(ARBHOME)/INCLUDE/smartptr.h
reader.o: $(ARBHOME)/INCLUDE/test_global.h

seq.o: ali.h
seq.o: defs.h
seq.o: fun.h
seq.o: global.h
seq.o: input_format.h
seq.o: prototypes.h
seq.o: reader.h
seq.o: seq.h
seq.o: $(ARBHOME)/INCLUDE/arb_assert.h
seq.o: $(ARBHOME)/INCLUDE/arb_core.h
seq.o: $(ARBHOME)/INCLUDE/arb_defs.h
seq.o: $(ARBHOME)/INCLUDE/arbtools.h
seq.o: $(ARBHOME)/INCLUDE/attributes.h
seq.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
seq.o: $(ARBHOME)/INCLUDE/cxxforward.h
seq.o: $(ARBHOME)/INCLUDE/dupstr.h
seq.o: $(ARBHOME)/INCLUDE/gccver.h
seq.o: $(ARBHOME)/INCLUDE/smartptr.h
seq.o: $(ARBHOME)/INCLUDE/test_global.h

util.o: defs.h
util.o: fun.h
util.o: global.h
util.o: input_format.h
util.o: prototypes.h
util.o: reader.h
util.o: seq.h
util.o: $(ARBHOME)/INCLUDE/arb_assert.h
util.o: $(ARBHOME)/INCLUDE/arb_core.h
util.o: $(ARBHOME)/INCLUDE/arb_defs.h
util.o: $(ARBHOME)/INCLUDE/arbtools.h
util.o: $(ARBHOME)/INCLUDE/attributes.h
util.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
util.o: $(ARBHOME)/INCLUDE/cxxforward.h
util.o: $(ARBHOME)/INCLUDE/dupstr.h
util.o: $(ARBHOME)/INCLUDE/gccver.h
util.o: $(ARBHOME)/INCLUDE/smartptr.h
util.o: $(ARBHOME)/INCLUDE/test_global.h

wrap.o: defs.h
wrap.o: fun.h
wrap.o: global.h
wrap.o: input_format.h
wrap.o: prototypes.h
wrap.o: reader.h
wrap.o: seq.h
wrap.o: wrap.h
wrap.o: $(ARBHOME)/INCLUDE/arb_assert.h
wrap.o: $(ARBHOME)/INCLUDE/arb_core.h
wrap.o: $(ARBHOME)/INCLUDE/arb_defs.h
wrap.o: $(ARBHOME)/INCLUDE/arbtools.h
wrap.o: $(ARBHOME)/INCLUDE/attributes.h
wrap.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
wrap.o: $(ARBHOME)/INCLUDE/cxxforward.h
wrap.o: $(ARBHOME)/INCLUDE/dupstr.h
wrap.o: $(ARBHOME)/INCLUDE/gccver.h
wrap.o: $(ARBHOME)/INCLUDE/smartptr.h
wrap.o: $(ARBHOME)/INCLUDE/test_global.h
