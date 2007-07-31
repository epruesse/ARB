# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .c .cxx .depend

OBJECTS = AC_main.o AC_tree.o AC_sequence.o DEBUG_tools.o AC_toys_n_tools.o

$(MAIN): $(OBJECTS)
	$(AR) $(MAIN) $(OBJECTS)

AC_sequence.o: AC_sequence.cxx
	CC -O $(cflags) -c $< $(CPPINCLUDES)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

clean:
	rm -f $(OBJECTS) 

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

AC_main.o: ac.hxx
AC_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
AC_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AC_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
AC_main.o: $(ARBHOME)/INCLUDE/arbdb.h
AC_main.o: $(ARBHOME)/INCLUDE/arbdbt.h

AC_sequence.o: ac.hxx
AC_sequence.o: $(ARBHOME)/INCLUDE/ad_prot.h
AC_sequence.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AC_sequence.o: $(ARBHOME)/INCLUDE/arb_assert.h
AC_sequence.o: $(ARBHOME)/INCLUDE/arbdb.h
AC_sequence.o: $(ARBHOME)/INCLUDE/arbdbt.h

AC_toys_n_tools.o: ac.hxx
AC_toys_n_tools.o: $(ARBHOME)/INCLUDE/ad_prot.h
AC_toys_n_tools.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AC_toys_n_tools.o: $(ARBHOME)/INCLUDE/arb_assert.h
AC_toys_n_tools.o: $(ARBHOME)/INCLUDE/arbdb.h
AC_toys_n_tools.o: $(ARBHOME)/INCLUDE/arbdbt.h

AC_tree.o: ac.hxx
AC_tree.o: $(ARBHOME)/INCLUDE/ad_prot.h
AC_tree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AC_tree.o: $(ARBHOME)/INCLUDE/arb_assert.h
AC_tree.o: $(ARBHOME)/INCLUDE/arbdb.h
AC_tree.o: $(ARBHOME)/INCLUDE/arbdbt.h

DEBUG_tools.o: ac.hxx
DEBUG_tools.o: $(ARBHOME)/INCLUDE/ad_prot.h
DEBUG_tools.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
DEBUG_tools.o: $(ARBHOME)/INCLUDE/arb_assert.h
DEBUG_tools.o: $(ARBHOME)/INCLUDE/arbdb.h
DEBUG_tools.o: $(ARBHOME)/INCLUDE/arbdbt.h
