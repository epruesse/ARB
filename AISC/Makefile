# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .depend
SOURCES = $(wildcard *.c)
OBJECTS = $(subst .c,.o,$(SOURCES))

BINARY=aisc

LOCAL_DEFINES=-DSIMPLE_ARB_ASSERT
LOCAL_MAKEDEPENDFLAGS=$(MAKEDEPENDFLAGS) $(LOCAL_DEFINES)

$(MAIN): proto
	$(MAKE) $(BINARY)

$(BINARY): $(OBJECTS)
	$(LINK_EXECUTABLE) $@ $(OBJECTS) $(EXECLIBS)

.c.o:
	$(A_CXX) -x c++ $(cflags) $(cxxflags) $(LOCAL_DEFINES) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

DEPENDS = $(OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(LOCAL_MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(LOCAL_MAKEDEPENDFLAGS) $< 2>/dev/null >$@

clean:
	rm -f $(OBJECTS) $(BINARY)

proto:
	((../AISC_MKPTPS/aisc_mkpt -P -G -w aisc_proto.h *.c >aisc_proto.h.tmp) 2>&1) | ../SOURCE_TOOLS/asan2msg.pl
	../SOURCE_TOOLS/mv_if_diff aisc_proto.h.tmp aisc_proto.h

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

aisc.o: aisc_def.h
aisc.o: aisc_inline.h
aisc.o: aisc_interpreter.h
aisc.o: aisc_location.h
aisc.o: aisc_parser.h
aisc.o: aisc_proto.h
aisc.o: aisc_token.h
aisc.o: $(ARBHOME)/INCLUDE/arb_assert.h
aisc.o: $(ARBHOME)/INCLUDE/arb_simple_assert.h
aisc.o: $(ARBHOME)/INCLUDE/arbtools.h
aisc.o: $(ARBHOME)/INCLUDE/attributes.h
aisc.o: $(ARBHOME)/INCLUDE/cxxforward.h
aisc.o: $(ARBHOME)/INCLUDE/dupstr.h
aisc.o: $(ARBHOME)/INCLUDE/gccver.h
aisc.o: $(ARBHOME)/INCLUDE/test_global.h

aisc_commands.o: aisc_def.h
aisc_commands.o: aisc_eval.h
aisc_commands.o: aisc_inline.h
aisc_commands.o: aisc_interpreter.h
aisc_commands.o: aisc_location.h
aisc_commands.o: aisc_parser.h
aisc_commands.o: aisc_proto.h
aisc_commands.o: aisc_token.h
aisc_commands.o: $(ARBHOME)/INCLUDE/arb_assert.h
aisc_commands.o: $(ARBHOME)/INCLUDE/arb_simple_assert.h
aisc_commands.o: $(ARBHOME)/INCLUDE/arbtools.h
aisc_commands.o: $(ARBHOME)/INCLUDE/attributes.h
aisc_commands.o: $(ARBHOME)/INCLUDE/cxxforward.h
aisc_commands.o: $(ARBHOME)/INCLUDE/dupstr.h
aisc_commands.o: $(ARBHOME)/INCLUDE/gccver.h
aisc_commands.o: $(ARBHOME)/INCLUDE/test_global.h

aisc_eval.o: aisc_def.h
aisc_eval.o: aisc_eval.h
aisc_eval.o: aisc_inline.h
aisc_eval.o: aisc_location.h
aisc_eval.o: aisc_proto.h
aisc_eval.o: $(ARBHOME)/INCLUDE/arb_assert.h
aisc_eval.o: $(ARBHOME)/INCLUDE/arb_simple_assert.h
aisc_eval.o: $(ARBHOME)/INCLUDE/arbtools.h
aisc_eval.o: $(ARBHOME)/INCLUDE/attributes.h
aisc_eval.o: $(ARBHOME)/INCLUDE/cxxforward.h
aisc_eval.o: $(ARBHOME)/INCLUDE/dupstr.h
aisc_eval.o: $(ARBHOME)/INCLUDE/gccver.h
aisc_eval.o: $(ARBHOME)/INCLUDE/test_global.h

aisc_mix.o: aisc_def.h
aisc_mix.o: aisc_inline.h
aisc_mix.o: aisc_interpreter.h
aisc_mix.o: aisc_location.h
aisc_mix.o: aisc_parser.h
aisc_mix.o: aisc_proto.h
aisc_mix.o: aisc_token.h
aisc_mix.o: $(ARBHOME)/INCLUDE/arb_assert.h
aisc_mix.o: $(ARBHOME)/INCLUDE/arb_simple_assert.h
aisc_mix.o: $(ARBHOME)/INCLUDE/arbtools.h
aisc_mix.o: $(ARBHOME)/INCLUDE/attributes.h
aisc_mix.o: $(ARBHOME)/INCLUDE/cxxforward.h
aisc_mix.o: $(ARBHOME)/INCLUDE/dupstr.h
aisc_mix.o: $(ARBHOME)/INCLUDE/gccver.h
aisc_mix.o: $(ARBHOME)/INCLUDE/test_global.h

aisc_parser.o: aisc_def.h
aisc_parser.o: aisc_inline.h
aisc_parser.o: aisc_location.h
aisc_parser.o: aisc_parser.h
aisc_parser.o: aisc_token.h
aisc_parser.o: $(ARBHOME)/INCLUDE/arb_assert.h
aisc_parser.o: $(ARBHOME)/INCLUDE/arb_simple_assert.h
aisc_parser.o: $(ARBHOME)/INCLUDE/arbtools.h
aisc_parser.o: $(ARBHOME)/INCLUDE/attributes.h
aisc_parser.o: $(ARBHOME)/INCLUDE/cxxforward.h
aisc_parser.o: $(ARBHOME)/INCLUDE/dupstr.h
aisc_parser.o: $(ARBHOME)/INCLUDE/gccver.h
aisc_parser.o: $(ARBHOME)/INCLUDE/test_global.h

aisc_var_ref.o: aisc_def.h
aisc_var_ref.o: aisc_inline.h
aisc_var_ref.o: aisc_interpreter.h
aisc_var_ref.o: aisc_location.h
aisc_var_ref.o: aisc_parser.h
aisc_var_ref.o: aisc_proto.h
aisc_var_ref.o: aisc_token.h
aisc_var_ref.o: $(ARBHOME)/INCLUDE/arb_assert.h
aisc_var_ref.o: $(ARBHOME)/INCLUDE/arb_simple_assert.h
aisc_var_ref.o: $(ARBHOME)/INCLUDE/arbtools.h
aisc_var_ref.o: $(ARBHOME)/INCLUDE/attributes.h
aisc_var_ref.o: $(ARBHOME)/INCLUDE/cxxforward.h
aisc_var_ref.o: $(ARBHOME)/INCLUDE/dupstr.h
aisc_var_ref.o: $(ARBHOME)/INCLUDE/gccver.h
aisc_var_ref.o: $(ARBHOME)/INCLUDE/test_global.h
