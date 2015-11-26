
.SUFFIXES: .o .cxx .depend .h .genh 

OBJECTS=$(subst .cxx,.o,$(wildcard *.cxx))

GENERATED_HEADERS= \
	arb_msg.h \
	arb_misc.h \
	arb_string.h \
	arb_signal.h \
	arb_file.h \
	arb_diff.h \
	arb_match.h \

LIB_SHARED=../lib/$(MAIN:.a=).$(SHARED_LIB_SUFFIX)

# MAIN contains .a (instead of wanted .so). Translate via do_...-target
it: do_$(MAIN)

do_CORE.a:
	@echo "Error: MAIN='$(MAIN)' is set wrong"
	false

do_libCORE.a: $(LIB_SHARED)

$(LIB_SHARED): $(OBJECTS)
	$(LINK_SHARED_LIB) $(LIB_SHARED) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(shared_cflags) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

.cxx.genh:
	../AISC_MKPTPS/aisc_mkpt -P -G -w $(subst .genh,.h,$@) $< > $@

%.genheader: %.genh
	../SOURCE_TOOLS/mv_if_diff $< $(subst .genheader,.h,$@)

proto: $(subst .h,.genheader,$(GENERATED_HEADERS))

clean:
	rm -f $(OBJECTS) *.a *.so

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

arb_cs.o: arb_assert.h
arb_cs.o: arb_core.h
arb_cs.o: arb_cs.h
arb_cs.o: arb_msg.h
arb_cs.o: arb_pattern.h
arb_cs.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_cs.o: $(ARBHOME)/INCLUDE/attributes.h
arb_cs.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_cs.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_cs.o: $(ARBHOME)/INCLUDE/gccver.h
arb_cs.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_cs.o: $(ARBHOME)/INCLUDE/test_global.h
arb_cs.o: $(ARBHOME)/INCLUDE/test_unit.h

arb_diff.o: arb_assert.h
arb_diff.o: arb_core.h
arb_diff.o: arb_diff.h
arb_diff.o: arb_file.h
arb_diff.o: arb_match.h
arb_diff.o: arb_msg.h
arb_diff.o: arb_string.h
arb_diff.o: $(ARBHOME)/INCLUDE/arb_str.h
arb_diff.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_diff.o: $(ARBHOME)/INCLUDE/attributes.h
arb_diff.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_diff.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_diff.o: $(ARBHOME)/INCLUDE/gccver.h
arb_diff.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_diff.o: $(ARBHOME)/INCLUDE/test_global.h
arb_diff.o: $(ARBHOME)/INCLUDE/test_unit.h

arb_file.o: arb_assert.h
arb_file.o: arb_core.h
arb_file.o: arb_file.h
arb_file.o: arb_msg.h
arb_file.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_file.o: $(ARBHOME)/INCLUDE/attributes.h
arb_file.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_file.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_file.o: $(ARBHOME)/INCLUDE/gccver.h
arb_file.o: $(ARBHOME)/INCLUDE/test_global.h
arb_file.o: $(ARBHOME)/INCLUDE/test_unit.h

arb_handlers.o: arb_assert.h
arb_handlers.o: arb_core.h
arb_handlers.o: arb_handlers.h
arb_handlers.o: arb_misc.h
arb_handlers.o: arb_msg.h
arb_handlers.o: $(ARBHOME)/INCLUDE/arb_algo.h
arb_handlers.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_handlers.o: $(ARBHOME)/INCLUDE/attributes.h
arb_handlers.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_handlers.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_handlers.o: $(ARBHOME)/INCLUDE/gccver.h
arb_handlers.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_handlers.o: $(ARBHOME)/INCLUDE/test_global.h

arb_match.o: arb_assert.h
arb_match.o: arb_core.h
arb_match.o: arb_match.h
arb_match.o: arb_msg.h
arb_match.o: arb_strbuf.h
arb_match.o: arb_string.h
arb_match.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_match.o: $(ARBHOME)/INCLUDE/attributes.h
arb_match.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_match.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_match.o: $(ARBHOME)/INCLUDE/gccver.h
arb_match.o: $(ARBHOME)/INCLUDE/test_global.h

arb_mem.o: arb_core.h
arb_mem.o: arb_mem.h
arb_mem.o: arb_msg.h
arb_mem.o: $(ARBHOME)/INCLUDE/attributes.h
arb_mem.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_mem.o: $(ARBHOME)/INCLUDE/gccver.h

arb_misc.o: arb_assert.h
arb_misc.o: arb_core.h
arb_misc.o: arb_misc.h
arb_misc.o: arb_msg.h
arb_misc.o: $(ARBHOME)/INCLUDE/attributes.h
arb_misc.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_misc.o: $(ARBHOME)/INCLUDE/gccver.h
arb_misc.o: $(ARBHOME)/INCLUDE/test_global.h

arb_msg.o: arb_assert.h
arb_msg.o: arb_core.h
arb_msg.o: arb_handlers.h
arb_msg.o: arb_msg.h
arb_msg.o: arb_msg_fwd.h
arb_msg.o: arb_strarray.h
arb_msg.o: arb_strbuf.h
arb_msg.o: arb_string.h
arb_msg.o: FileContent.h
arb_msg.o: $(ARBHOME)/INCLUDE/arb_backtrace.h
arb_msg.o: $(ARBHOME)/INCLUDE/arb_defs.h
arb_msg.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_msg.o: $(ARBHOME)/INCLUDE/attributes.h
arb_msg.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_msg.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_msg.o: $(ARBHOME)/INCLUDE/gccver.h
arb_msg.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_msg.o: $(ARBHOME)/INCLUDE/test_global.h
arb_msg.o: $(ARBHOME)/INCLUDE/test_unit.h

arb_pattern.o: arb_assert.h
arb_pattern.o: arb_core.h
arb_pattern.o: arb_msg.h
arb_pattern.o: arb_pattern.h
arb_pattern.o: arb_strbuf.h
arb_pattern.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_pattern.o: $(ARBHOME)/INCLUDE/attributes.h
arb_pattern.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_pattern.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_pattern.o: $(ARBHOME)/INCLUDE/gccver.h
arb_pattern.o: $(ARBHOME)/INCLUDE/test_global.h
arb_pattern.o: $(ARBHOME)/INCLUDE/test_unit.h

arb_progress.o: arb_assert.h
arb_progress.o: arb_core.h
arb_progress.o: arb_handlers.h
arb_progress.o: arb_msg.h
arb_progress.o: arb_progress.h
arb_progress.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_progress.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_progress.o: $(ARBHOME)/INCLUDE/attributes.h
arb_progress.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_progress.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_progress.o: $(ARBHOME)/INCLUDE/gccver.h
arb_progress.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_progress.o: $(ARBHOME)/INCLUDE/test_global.h

arb_signal.o: arb_assert.h
arb_signal.o: arb_core.h
arb_signal.o: arb_msg.h
arb_signal.o: arb_signal.h
arb_signal.o: $(ARBHOME)/INCLUDE/arb_backtrace.h
arb_signal.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_signal.o: $(ARBHOME)/INCLUDE/attributes.h
arb_signal.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_signal.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_signal.o: $(ARBHOME)/INCLUDE/gccver.h
arb_signal.o: $(ARBHOME)/INCLUDE/SigHandler.h
arb_signal.o: $(ARBHOME)/INCLUDE/SuppressOutput.h
arb_signal.o: $(ARBHOME)/INCLUDE/test_global.h
arb_signal.o: $(ARBHOME)/INCLUDE/test_unit.h
arb_signal.o: $(ARBHOME)/INCLUDE/valgrind.h

arb_sort.o: arb_assert.h
arb_sort.o: arb_core.h
arb_sort.o: arb_sort.h
arb_sort.o: $(ARBHOME)/INCLUDE/attributes.h
arb_sort.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_sort.o: $(ARBHOME)/INCLUDE/gccver.h
arb_sort.o: $(ARBHOME)/INCLUDE/test_global.h

arb_strarray.o: arb_assert.h
arb_strarray.o: arb_core.h
arb_strarray.o: arb_sort.h
arb_strarray.o: arb_strarray.h
arb_strarray.o: arb_strbuf.h
arb_strarray.o: arb_string.h
arb_strarray.o: $(ARBHOME)/INCLUDE/arb_str.h
arb_strarray.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_strarray.o: $(ARBHOME)/INCLUDE/attributes.h
arb_strarray.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_strarray.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_strarray.o: $(ARBHOME)/INCLUDE/gccver.h
arb_strarray.o: $(ARBHOME)/INCLUDE/test_global.h
arb_strarray.o: $(ARBHOME)/INCLUDE/test_unit.h

arb_strbuf.o: arb_assert.h
arb_strbuf.o: arb_core.h
arb_strbuf.o: arb_strbuf.h
arb_strbuf.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_strbuf.o: $(ARBHOME)/INCLUDE/attributes.h
arb_strbuf.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_strbuf.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_strbuf.o: $(ARBHOME)/INCLUDE/gccver.h
arb_strbuf.o: $(ARBHOME)/INCLUDE/test_global.h

arb_string.o: arb_assert.h
arb_string.o: arb_core.h
arb_string.o: arb_string.h
arb_string.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_string.o: $(ARBHOME)/INCLUDE/attributes.h
arb_string.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_string.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_string.o: $(ARBHOME)/INCLUDE/gccver.h
arb_string.o: $(ARBHOME)/INCLUDE/Keeper.h
arb_string.o: $(ARBHOME)/INCLUDE/test_global.h
arb_string.o: $(ARBHOME)/INCLUDE/test_unit.h

BufferedFileReader.o: arb_assert.h
BufferedFileReader.o: arb_core.h
BufferedFileReader.o: BufferedFileReader.h
BufferedFileReader.o: $(ARBHOME)/INCLUDE/arbtools.h
BufferedFileReader.o: $(ARBHOME)/INCLUDE/attributes.h
BufferedFileReader.o: $(ARBHOME)/INCLUDE/cxxforward.h
BufferedFileReader.o: $(ARBHOME)/INCLUDE/dupstr.h
BufferedFileReader.o: $(ARBHOME)/INCLUDE/gccver.h
BufferedFileReader.o: $(ARBHOME)/INCLUDE/smartptr.h
BufferedFileReader.o: $(ARBHOME)/INCLUDE/test_global.h

FileContent.o: arb_assert.h
FileContent.o: arb_core.h
FileContent.o: arb_file.h
FileContent.o: arb_msg.h
FileContent.o: arb_strarray.h
FileContent.o: arb_string.h
FileContent.o: BufferedFileReader.h
FileContent.o: FileContent.h
FileContent.o: $(ARBHOME)/INCLUDE/arbtools.h
FileContent.o: $(ARBHOME)/INCLUDE/attributes.h
FileContent.o: $(ARBHOME)/INCLUDE/cxxforward.h
FileContent.o: $(ARBHOME)/INCLUDE/dupstr.h
FileContent.o: $(ARBHOME)/INCLUDE/gccver.h
FileContent.o: $(ARBHOME)/INCLUDE/test_global.h
FileContent.o: $(ARBHOME)/INCLUDE/test_unit.h

MultiFileReader.o: arb_assert.h
MultiFileReader.o: arb_core.h
MultiFileReader.o: arb_msg.h
MultiFileReader.o: arb_strarray.h
MultiFileReader.o: BufferedFileReader.h
MultiFileReader.o: MultiFileReader.h
MultiFileReader.o: $(ARBHOME)/INCLUDE/arbtools.h
MultiFileReader.o: $(ARBHOME)/INCLUDE/attributes.h
MultiFileReader.o: $(ARBHOME)/INCLUDE/cxxforward.h
MultiFileReader.o: $(ARBHOME)/INCLUDE/dupstr.h
MultiFileReader.o: $(ARBHOME)/INCLUDE/gccver.h
MultiFileReader.o: $(ARBHOME)/INCLUDE/test_global.h
MultiFileReader.o: $(ARBHOME)/INCLUDE/test_unit.h

pos_range.o: arb_assert.h
pos_range.o: arb_core.h
pos_range.o: arb_msg.h
pos_range.o: pos_range.h
pos_range.o: $(ARBHOME)/INCLUDE/arbtools.h
pos_range.o: $(ARBHOME)/INCLUDE/attributes.h
pos_range.o: $(ARBHOME)/INCLUDE/cxxforward.h
pos_range.o: $(ARBHOME)/INCLUDE/dupstr.h
pos_range.o: $(ARBHOME)/INCLUDE/gccver.h
pos_range.o: $(ARBHOME)/INCLUDE/smartptr.h
pos_range.o: $(ARBHOME)/INCLUDE/test_global.h
pos_range.o: $(ARBHOME)/INCLUDE/test_unit.h
