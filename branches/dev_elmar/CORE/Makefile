
.SUFFIXES: .o .cxx .depend .h .genh 

OBJECTS=$(subst .cxx,.o,$(wildcard *.cxx))

GENERATED_HEADERS= \
	arb_msg.h \
	arb_string.h \
	arb_signal.h \
	arb_file.h \

$(MAIN): $(OBJECTS)
	$(LINK_SHARED_LIB) $(@:.a=).$(SHARED_LIB_SUFFIX) $(OBJECTS)
	touch $@

.cxx.o:
	$(CPPLIB) $(cflags) -c $< $(AINCLUDES) $(POST_COMPILE)

.cxx.genh:
	../AISC_MKPTPS/aisc_mkpt -P -G -A -w $(subst .genh,.h,$@) $< > $@

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

arb_cs.o: arb_core.h
arb_cs.o: arb_cs.h
arb_cs.o: arb_msg.h
arb_cs.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_cs.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_cs.o: $(ARBHOME)/INCLUDE/attributes.h
arb_cs.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_cs.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_cs.o: $(ARBHOME)/INCLUDE/test_global.h

arb_file.o: arb_core.h
arb_file.o: arb_file.h
arb_file.o: arb_msg.h
arb_file.o: $(ARBHOME)/INCLUDE/attributes.h
arb_file.o: $(ARBHOME)/INCLUDE/dupstr.h

arb_handlers.o: arb_core.h
arb_handlers.o: arb_handlers.h
arb_handlers.o: arb_msg.h
arb_handlers.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_handlers.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_handlers.o: $(ARBHOME)/INCLUDE/attributes.h
arb_handlers.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_handlers.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_handlers.o: $(ARBHOME)/INCLUDE/test_global.h

arb_msg.o: arb_core.h
arb_msg.o: arb_handlers.h
arb_msg.o: arb_msg.h
arb_msg.o: arb_string.h
arb_msg.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_msg.o: $(ARBHOME)/INCLUDE/arb_backtrace.h
arb_msg.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_msg.o: $(ARBHOME)/INCLUDE/attributes.h
arb_msg.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_msg.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_msg.o: $(ARBHOME)/INCLUDE/test_global.h

arb_progress.o: arb_core.h
arb_progress.o: arb_handlers.h
arb_progress.o: arb_msg.h
arb_progress.o: arb_progress.h
arb_progress.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_progress.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_progress.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_progress.o: $(ARBHOME)/INCLUDE/attributes.h
arb_progress.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_progress.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_progress.o: $(ARBHOME)/INCLUDE/test_global.h

arb_signal.o: arb_core.h
arb_signal.o: arb_msg.h
arb_signal.o: arb_signal.h
arb_signal.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_signal.o: $(ARBHOME)/INCLUDE/arb_backtrace.h
arb_signal.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_signal.o: $(ARBHOME)/INCLUDE/attributes.h
arb_signal.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_signal.o: $(ARBHOME)/INCLUDE/SigHandler.h
arb_signal.o: $(ARBHOME)/INCLUDE/SuppressOutput.h
arb_signal.o: $(ARBHOME)/INCLUDE/test_global.h
arb_signal.o: $(ARBHOME)/INCLUDE/valgrind.h

arb_sort.o: arb_sort.h

arb_strarray.o: arb_core.h
arb_strarray.o: arb_sort.h
arb_strarray.o: arb_strarray.h
arb_strarray.o: arb_strbuf.h
arb_strarray.o: arb_string.h
arb_strarray.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_strarray.o: $(ARBHOME)/INCLUDE/arb_str.h
arb_strarray.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_strarray.o: $(ARBHOME)/INCLUDE/attributes.h
arb_strarray.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_strarray.o: $(ARBHOME)/INCLUDE/test_global.h
arb_strarray.o: $(ARBHOME)/INCLUDE/test_unit.h

arb_strbuf.o: arb_core.h
arb_strbuf.o: arb_strbuf.h
arb_strbuf.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_strbuf.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_strbuf.o: $(ARBHOME)/INCLUDE/attributes.h
arb_strbuf.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_strbuf.o: $(ARBHOME)/INCLUDE/test_global.h

arb_string.o: arb_core.h
arb_string.o: arb_string.h
arb_string.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_string.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_string.o: $(ARBHOME)/INCLUDE/attributes.h
arb_string.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_string.o: $(ARBHOME)/INCLUDE/test_global.h
arb_string.o: $(ARBHOME)/INCLUDE/test_unit.h
