# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = db_scanner.o 
OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES) $(POST_COMPILE)

.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES) $(POST_COMPILE)

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

db_scanner.o: db_scanner.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/ad_prot.h
db_scanner.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
db_scanner.o: $(ARBHOME)/INCLUDE/arb_assert.h
db_scanner.o: $(ARBHOME)/INCLUDE/arb_core.h
db_scanner.o: $(ARBHOME)/INCLUDE/arb_error.h
db_scanner.o: $(ARBHOME)/INCLUDE/arb_msg.h
db_scanner.o: $(ARBHOME)/INCLUDE/arb_str.h
db_scanner.o: $(ARBHOME)/INCLUDE/arb_string.h
db_scanner.o: $(ARBHOME)/INCLUDE/arbdb.h
db_scanner.o: $(ARBHOME)/INCLUDE/arbdb_base.h
db_scanner.o: $(ARBHOME)/INCLUDE/arbdbt.h
db_scanner.o: $(ARBHOME)/INCLUDE/arbtools.h
db_scanner.o: $(ARBHOME)/INCLUDE/attributes.h
db_scanner.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_base.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_root.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_select.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_status.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_window.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/awt.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/cb.h
db_scanner.o: $(ARBHOME)/INCLUDE/dupstr.h
db_scanner.o: $(ARBHOME)/INCLUDE/smartptr.h
db_scanner.o: $(ARBHOME)/INCLUDE/test_global.h
