# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .cxx .depend

CPP_OBJECTS = a3.o a3_seq.o a3_ptree1.o a3_ptree2.o a3_helix.o a3_darray.o a3_ali.o a3_matrix.o a3_arbdb.o a3_bihelix.o
C_OBJECTS   = a3_basen.o
OBJECTS = $(CPP_OBJECTS) $(C_OBJECTS)

$(MAIN): $(OBJECTS)
	$(AR) $(MAIN) $(OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

.c.o:
	$(ACC) $(cflags) -c $< $(CPPINCLUDES)

clean:
	@-rm $(OBJECTS)

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

a3.o: a3_ali.hxx
a3.o: a3_arbdb.hxx
a3.o: a3_bihelix.hxx
a3.o: a3_darray.hxx
a3.o: a3_helix.hxx
a3.o: a3_ptree.hxx
a3.o: a3_seq.hxx
a3.o: a3_types.h

a3_ali.o: a3_ali.hxx
a3_ali.o: a3_darray.hxx
a3_ali.o: a3_helix.hxx
a3_ali.o: a3_ptree.hxx
a3_ali.o: a3_seq.hxx
a3_ali.o: a3_types.h

a3_arbdb.o: a3_arbdb.hxx

a3_basen.o: a3_basen.h

a3_bihelix.o: a3_arbdb.hxx
a3_bihelix.o: a3_bihelix.hxx

a3_darray.o: a3_darray.hxx

a3_helix.o: a3_ali.hxx
a3_helix.o: a3_darray.hxx
a3_helix.o: a3_helix.hxx
a3_helix.o: a3_matrix.hxx
a3_helix.o: a3_ptree.hxx
a3_helix.o: a3_seq.hxx
a3_helix.o: a3_types.h

a3_matrix.o: a3_darray.hxx
a3_matrix.o: a3_matrix.hxx

a3_ptree1.o: a3_basen.h
a3_ptree1.o: a3_ptree.hxx
a3_ptree1.o: a3_seq.hxx
a3_ptree1.o: a3_types.h

a3_ptree2.o: a3_basen.h
a3_ptree2.o: a3_ptree.hxx
a3_ptree2.o: a3_seq.hxx
a3_ptree2.o: a3_types.h

a3_seq.o: a3_basen.h
a3_seq.o: a3_seq.hxx
a3_seq.o: a3_types.h
