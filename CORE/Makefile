
.SUFFIXES: .o .cxx .depend .h .genh

OBJECTS=$(subst .cxx,.o,$(wildcard *.cxx))

$(MAIN): $(OBJECTS)
	$(LINK_SHARED_LIB) $(@:.a=).$(SHARED_LIB_SUFFIX) $(OBJECTS)
	touch $@

.cxx.o:
	$(CPPLIB) $(cflags) -c $< $(AINCLUDES) $(POST_COMPILE)

proto: 
	../MAKEBIN/aisc_mkpt -P -G -A -w arb_msg.h    arb_msg.cxx    > arb_msg.genh
	../MAKEBIN/aisc_mkpt -P -G -A -w arb_string.h arb_string.cxx > arb_string.genh
	../MAKEBIN/aisc_mkpt -P -G -A -w arb_signal.h arb_signal.cxx > arb_signal.genh
	../SOURCE_TOOLS/mv_if_diff arb_msg.genh    arb_msg.h
	../SOURCE_TOOLS/mv_if_diff arb_string.genh arb_string.h
	../SOURCE_TOOLS/mv_if_diff arb_signal.genh arb_signal.h

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

arb_handlers.o: arb_core.h
arb_handlers.o: arb_handlers.h
arb_handlers.o: arb_msg.h
arb_handlers.o: $(ARBHOME)/INCLUDE/arb_assert.h
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
arb_signal.o: $(ARBHOME)/INCLUDE/attributes.h
arb_signal.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_signal.o: $(ARBHOME)/INCLUDE/SigHandler.h
arb_signal.o: $(ARBHOME)/INCLUDE/SuppressOutput.h
arb_signal.o: $(ARBHOME)/INCLUDE/test_global.h
arb_signal.o: $(ARBHOME)/INCLUDE/valgrind.h

arb_string.o: arb_core.h
arb_string.o: arb_string.h
arb_string.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_string.o: $(ARBHOME)/INCLUDE/attributes.h
arb_string.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_string.o: $(ARBHOME)/INCLUDE/test_global.h
