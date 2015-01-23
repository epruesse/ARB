# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt
.SUFFIXES: .o .c .cxx .depend
OBJECTS = servercntrl.o
$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(@) $(OBJECTS)
.c.o:
	$(A_CC) $(cflags) -c $< $(CC_INCLUDES) $(POST_COMPILE) $<
.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

proto:
	../AISC_MKPTPS/aisc_mkpt -P -w servercntrl.h $(OBJECTS:.o=.cxx) >servercntrl.h.tmp
	../SOURCE_TOOLS/mv_if_diff servercntrl.h.tmp servercntrl.h

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

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

servercntrl.o: servercntrl.h
servercntrl.o: $(ARBHOME)/INCLUDE/ad_prot.h
servercntrl.o: $(ARBHOME)/INCLUDE/aisc_global.h
servercntrl.o: $(ARBHOME)/INCLUDE/arb_assert.h
servercntrl.o: $(ARBHOME)/INCLUDE/arb_core.h
servercntrl.o: $(ARBHOME)/INCLUDE/arb_error.h
servercntrl.o: $(ARBHOME)/INCLUDE/arb_file.h
servercntrl.o: $(ARBHOME)/INCLUDE/arb_msg.h
servercntrl.o: $(ARBHOME)/INCLUDE/arb_sleep.h
servercntrl.o: $(ARBHOME)/INCLUDE/arb_string.h
servercntrl.o: $(ARBHOME)/INCLUDE/arbdb.h
servercntrl.o: $(ARBHOME)/INCLUDE/arbdb_base.h
servercntrl.o: $(ARBHOME)/INCLUDE/arbtools.h
servercntrl.o: $(ARBHOME)/INCLUDE/attributes.h
servercntrl.o: $(ARBHOME)/INCLUDE/bytestring.h
servercntrl.o: $(ARBHOME)/INCLUDE/client.h
servercntrl.o: $(ARBHOME)/INCLUDE/client_privat.h
servercntrl.o: $(ARBHOME)/INCLUDE/client_types.h
servercntrl.o: $(ARBHOME)/INCLUDE/cxxforward.h
servercntrl.o: $(ARBHOME)/INCLUDE/dupstr.h
servercntrl.o: $(ARBHOME)/INCLUDE/gccver.h
servercntrl.o: $(ARBHOME)/INCLUDE/SigHandler.h
servercntrl.o: $(ARBHOME)/INCLUDE/smartptr.h
servercntrl.o: $(ARBHOME)/INCLUDE/test_global.h
servercntrl.o: $(ARBHOME)/INCLUDE/ut_valgrinded.h
