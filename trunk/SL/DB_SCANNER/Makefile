# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = db_scanner.o 
OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(AR) $(MAIN) $(OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES)

clean:
	@-rm $(OBJECTS)
	@-rm *.a

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
db_scanner.o: $(ARBHOME)/INCLUDE/arbdb.h
db_scanner.o: $(ARBHOME)/INCLUDE/arbdbt.h
db_scanner.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_device.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_root.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/aw_window.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/awt.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/awt_changekey.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/awtlocal.hxx
db_scanner.o: $(ARBHOME)/INCLUDE/inline.h
