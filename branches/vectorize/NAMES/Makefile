# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt
.SUFFIXES: .o .cxx .c .depend
OBJECTS = names.o
$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)
.c.o:
	$(A_CC) $(cflags) -c $< $(CC_INCLUDES) $(POST_COMPILE) $<
.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

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

names.o: names.h
names.o: $(ARBHOME)/INCLUDE/ad_prot.h
names.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
names.o: $(ARBHOME)/INCLUDE/aisc_global.h
names.o: $(ARBHOME)/INCLUDE/arb_assert.h
names.o: $(ARBHOME)/INCLUDE/arb_core.h
names.o: $(ARBHOME)/INCLUDE/arb_error.h
names.o: $(ARBHOME)/INCLUDE/arb_file.h
names.o: $(ARBHOME)/INCLUDE/arb_msg.h
names.o: $(ARBHOME)/INCLUDE/arb_sleep.h
names.o: $(ARBHOME)/INCLUDE/arb_string.h
names.o: $(ARBHOME)/INCLUDE/arbdb.h
names.o: $(ARBHOME)/INCLUDE/arbdb_base.h
names.o: $(ARBHOME)/INCLUDE/arbtools.h
names.o: $(ARBHOME)/INCLUDE/attributes.h
names.o: $(ARBHOME)/INCLUDE/bytestring.h
names.o: $(ARBHOME)/INCLUDE/client.h
names.o: $(ARBHOME)/INCLUDE/client_types.h
names.o: $(ARBHOME)/INCLUDE/cxxforward.h
names.o: $(ARBHOME)/INCLUDE/dupstr.h
names.o: $(ARBHOME)/INCLUDE/gccver.h
names.o: $(ARBHOME)/INCLUDE/names_client.h
names.o: $(ARBHOME)/INCLUDE/names_prototypes.h
names.o: $(ARBHOME)/INCLUDE/names_server.h
names.o: $(ARBHOME)/INCLUDE/server.h
names.o: $(ARBHOME)/INCLUDE/servercntrl.h
names.o: $(ARBHOME)/INCLUDE/smartptr.h
names.o: $(ARBHOME)/INCLUDE/struct_man.h
names.o: $(ARBHOME)/INCLUDE/test_global.h
