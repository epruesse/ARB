.SUFFIXES: .o .cxx .depend

OBJS = CT_part.o CT_hash.o CT_ntree.o CT_rbtree.o CT_dtree.o CT_ctree.o CT_mem.o

$(MAIN): $(OBJS)
	$(AR) $(MAIN) $(OBJS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)
clean:
	rm -f $(OBJS) *.a

DEPENDS = $(OBJS:.o=.depend)
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

CT_ctree.o: CT_dtree.hxx
CT_ctree.o: CT_hash.hxx
CT_ctree.o: CT_ntree.hxx
CT_ctree.o: CT_part.hxx
CT_ctree.o: CT_rbtree.hxx
CT_ctree.o: $(ARBHOME)/INCLUDE/ad_prot.h
CT_ctree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
CT_ctree.o: $(ARBHOME)/INCLUDE/arb_assert.h
CT_ctree.o: $(ARBHOME)/INCLUDE/arbdb.h
CT_ctree.o: $(ARBHOME)/INCLUDE/arbdbt.h

CT_dtree.o: CT_hash.hxx
CT_dtree.o: CT_part.hxx
CT_dtree.o: $(ARBHOME)/INCLUDE/ad_prot.h
CT_dtree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
CT_dtree.o: $(ARBHOME)/INCLUDE/arb_assert.h
CT_dtree.o: $(ARBHOME)/INCLUDE/arbdb.h
CT_dtree.o: $(ARBHOME)/INCLUDE/arbdbt.h

CT_hash.o: CT_hash.hxx
CT_hash.o: CT_mem.hxx
CT_hash.o: CT_ntree.hxx
CT_hash.o: CT_part.hxx
CT_hash.o: $(ARBHOME)/INCLUDE/ad_prot.h
CT_hash.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
CT_hash.o: $(ARBHOME)/INCLUDE/arb_assert.h
CT_hash.o: $(ARBHOME)/INCLUDE/arbdb.h
CT_hash.o: $(ARBHOME)/INCLUDE/arbdbt.h

CT_ntree.o: CT_mem.hxx
CT_ntree.o: CT_ntree.hxx
CT_ntree.o: CT_part.hxx
CT_ntree.o: $(ARBHOME)/INCLUDE/ad_prot.h
CT_ntree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
CT_ntree.o: $(ARBHOME)/INCLUDE/arb_assert.h
CT_ntree.o: $(ARBHOME)/INCLUDE/arbdb.h
CT_ntree.o: $(ARBHOME)/INCLUDE/arbdbt.h

CT_part.o: CT_mem.hxx
CT_part.o: CT_part.hxx
CT_part.o: $(ARBHOME)/INCLUDE/ad_prot.h
CT_part.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
CT_part.o: $(ARBHOME)/INCLUDE/arb_assert.h
CT_part.o: $(ARBHOME)/INCLUDE/arbdb.h
CT_part.o: $(ARBHOME)/INCLUDE/arbdbt.h

CT_rbtree.o: CT_mem.hxx
CT_rbtree.o: CT_ntree.hxx
CT_rbtree.o: CT_part.hxx
CT_rbtree.o: CT_rbtree.hxx
CT_rbtree.o: $(ARBHOME)/INCLUDE/ad_prot.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/arb_assert.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/arbdb.h
CT_rbtree.o: $(ARBHOME)/INCLUDE/arbdbt.h
