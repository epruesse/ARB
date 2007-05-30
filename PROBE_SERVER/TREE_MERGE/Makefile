# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .depend

# --------------------------------------------------------------------------------

# --------------------------------------------------------------------------------

CPP_OBJECTS = tree_merge.o

all : $(MAIN)

$(MAIN): $(CPP_OBJECTS)
	$(XAR) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c -o $@ $(@:.o=.cxx)  $(CPPINCLUDES)

clean:
	rm -f $(CPP_OBJECTS) *.a

DEPENDS = $(CPP_OBJECTS:.o=.depend)
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

tree_merge.o: ../common.h
tree_merge.o: $(ARBHOME)/INCLUDE/ad_prot.h
tree_merge.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
tree_merge.o: $(ARBHOME)/INCLUDE/arb_assert.h
tree_merge.o: $(ARBHOME)/INCLUDE/arbdb.h
tree_merge.o: $(ARBHOME)/INCLUDE/arbdbt.h
tree_merge.o: $(ARBHOME)/INCLUDE/smartptr.h
