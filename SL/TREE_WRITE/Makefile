# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = TreeWrite.o
OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES)

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

TreeWrite.o: TreeWrite.h
TreeWrite.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
TreeWrite.o: $(ARBHOME)/INCLUDE/ad_prot.h
TreeWrite.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
TreeWrite.o: $(ARBHOME)/INCLUDE/arb_assert.h
TreeWrite.o: $(ARBHOME)/INCLUDE/arb_core.h
TreeWrite.o: $(ARBHOME)/INCLUDE/arb_error.h
TreeWrite.o: $(ARBHOME)/INCLUDE/arbdb.h
TreeWrite.o: $(ARBHOME)/INCLUDE/arbdb_base.h
TreeWrite.o: $(ARBHOME)/INCLUDE/arbdbt.h
TreeWrite.o: $(ARBHOME)/INCLUDE/arbtools.h
TreeWrite.o: $(ARBHOME)/INCLUDE/attributes.h
TreeWrite.o: $(ARBHOME)/INCLUDE/aw_root.hxx
TreeWrite.o: $(ARBHOME)/INCLUDE/awt_nds.hxx
TreeWrite.o: $(ARBHOME)/INCLUDE/dupstr.h
TreeWrite.o: $(ARBHOME)/INCLUDE/smartptr.h
TreeWrite.o: $(ARBHOME)/INCLUDE/xml.hxx
