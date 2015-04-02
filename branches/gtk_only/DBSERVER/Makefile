# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt
.SUFFIXES: .o .c .cxx .depend
OBJECTS = db_server.o

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

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

db_server.o: $(ARBHOME)/INCLUDE/ad_cb.h
db_server.o: $(ARBHOME)/INCLUDE/ad_cb_prot.h
db_server.o: $(ARBHOME)/INCLUDE/ad_prot.h
db_server.o: $(ARBHOME)/INCLUDE/arb_assert.h
db_server.o: $(ARBHOME)/INCLUDE/arb_core.h
db_server.o: $(ARBHOME)/INCLUDE/arb_diff.h
db_server.o: $(ARBHOME)/INCLUDE/arb_error.h
db_server.o: $(ARBHOME)/INCLUDE/arb_file.h
db_server.o: $(ARBHOME)/INCLUDE/arb_msg.h
db_server.o: $(ARBHOME)/INCLUDE/arb_sleep.h
db_server.o: $(ARBHOME)/INCLUDE/arb_string.h
db_server.o: $(ARBHOME)/INCLUDE/arbdb.h
db_server.o: $(ARBHOME)/INCLUDE/arbdb_base.h
db_server.o: $(ARBHOME)/INCLUDE/arbtools.h
db_server.o: $(ARBHOME)/INCLUDE/attributes.h
db_server.o: $(ARBHOME)/INCLUDE/cb.h
db_server.o: $(ARBHOME)/INCLUDE/cb_base.h
db_server.o: $(ARBHOME)/INCLUDE/cbtypes.h
db_server.o: $(ARBHOME)/INCLUDE/cxxforward.h
db_server.o: $(ARBHOME)/INCLUDE/dupstr.h
db_server.o: $(ARBHOME)/INCLUDE/gccver.h
db_server.o: $(ARBHOME)/INCLUDE/servercntrl.h
db_server.o: $(ARBHOME)/INCLUDE/smartptr.h
db_server.o: $(ARBHOME)/INCLUDE/static_assert.h
db_server.o: $(ARBHOME)/INCLUDE/test_global.h
db_server.o: $(ARBHOME)/INCLUDE/test_unit.h
db_server.o: $(ARBHOME)/INCLUDE/ttypes.h
db_server.o: $(ARBHOME)/INCLUDE/ut_valgrinded.h
