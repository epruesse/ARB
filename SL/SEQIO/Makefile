# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = \
	seq_export.o \
	seqio.o \

OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from SL)

seq_export.o: seqio.hxx
seq_export.o: $(ARBHOME)/INCLUDE/ad_prot.h
seq_export.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
seq_export.o: $(ARBHOME)/INCLUDE/AP_filter.hxx
seq_export.o: $(ARBHOME)/INCLUDE/arb_assert.h
seq_export.o: $(ARBHOME)/INCLUDE/arb_core.h
seq_export.o: $(ARBHOME)/INCLUDE/arb_diff.h
seq_export.o: $(ARBHOME)/INCLUDE/arb_error.h
seq_export.o: $(ARBHOME)/INCLUDE/arb_file.h
seq_export.o: $(ARBHOME)/INCLUDE/arb_msg.h
seq_export.o: $(ARBHOME)/INCLUDE/arb_progress.h
seq_export.o: $(ARBHOME)/INCLUDE/arb_strarray.h
seq_export.o: $(ARBHOME)/INCLUDE/arb_string.h
seq_export.o: $(ARBHOME)/INCLUDE/arbdb.h
seq_export.o: $(ARBHOME)/INCLUDE/arbdb_base.h
seq_export.o: $(ARBHOME)/INCLUDE/arbdbt.h
seq_export.o: $(ARBHOME)/INCLUDE/arbtools.h
seq_export.o: $(ARBHOME)/INCLUDE/attributes.h
seq_export.o: $(ARBHOME)/INCLUDE/cxxforward.h
seq_export.o: $(ARBHOME)/INCLUDE/downcast.h
seq_export.o: $(ARBHOME)/INCLUDE/dupstr.h
seq_export.o: $(ARBHOME)/INCLUDE/gccver.h
seq_export.o: $(ARBHOME)/INCLUDE/smartptr.h
seq_export.o: $(ARBHOME)/INCLUDE/static_assert.h
seq_export.o: $(ARBHOME)/INCLUDE/test_global.h
seq_export.o: $(ARBHOME)/INCLUDE/test_unit.h
seq_export.o: $(ARBHOME)/INCLUDE/xml.hxx

seqio.o: seqio.hxx
seqio.o: $(ARBHOME)/INCLUDE/ad_prot.h
seqio.o: $(ARBHOME)/INCLUDE/arb_assert.h
seqio.o: $(ARBHOME)/INCLUDE/arb_core.h
seqio.o: $(ARBHOME)/INCLUDE/arb_error.h
seqio.o: $(ARBHOME)/INCLUDE/arb_msg.h
seqio.o: $(ARBHOME)/INCLUDE/arb_string.h
seqio.o: $(ARBHOME)/INCLUDE/arbdb.h
seqio.o: $(ARBHOME)/INCLUDE/arbdb_base.h
seqio.o: $(ARBHOME)/INCLUDE/arbtools.h
seqio.o: $(ARBHOME)/INCLUDE/attributes.h
seqio.o: $(ARBHOME)/INCLUDE/cxxforward.h
seqio.o: $(ARBHOME)/INCLUDE/dupstr.h
seqio.o: $(ARBHOME)/INCLUDE/gccver.h
seqio.o: $(ARBHOME)/INCLUDE/smartptr.h
seqio.o: $(ARBHOME)/INCLUDE/test_global.h
