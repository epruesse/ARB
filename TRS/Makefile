
.SUFFIXES: .o .c .cxx .depend

OBJECTS = trs_hash.o tree_lib.o trs_server.o

$(ARBHOME)/bin/trs_server: $(OBJECTS)
	$(CPP) -o $@ $(STATIC) $(OBJECTS)


# $(OBJECTS:%.o=$(ARBHOME)/irsbin/%)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

proto:
	rm -f trs_proto.h
	echo "#define P_(s) s" > trs_proto.h
	../MAKEBIN/aisc_mkpt $(OBJECTS:.o=.cxx) |awk '$$2 ~ /TRS/ {print;}' >>trs_proto.h
	../MAKEBIN/aisc_mkpt $(OBJECTS:.o=.cxx) |awk '$$2 ~ /T2J/ {print;}' |grep -v ':' >>trs_proto.h
	echo "#undef P_" >>trs_proto.h


$(ARBHOME)/irsbin/%: %.cxx
	$(CPP) $(cflags) -o $@ $< $(CPPINCLUDES) $(LIB2)

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

tree_lib.o: tree_lib.hxx
tree_lib.o: trs_proto.h
tree_lib.o: $(ARBHOME)/INCLUDE/ad_prot.h
tree_lib.o: $(ARBHOME)/INCLUDE/arb_assert.h
tree_lib.o: $(ARBHOME)/INCLUDE/arbdb.h
tree_lib.o: $(ARBHOME)/INCLUDE/cat_tree.hxx

trs_server.o: tree_lib.hxx
trs_server.o: trs_proto.h
trs_server.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
