
.SUFFIXES: .o .c .cxx

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

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(OBJECTS:.o=.c*)

# DO NOT DELETE THIS LINE -- make depend depends on it.

trs_hash.o:  
trs_hash.o:  
trs_hash.o:  
trs_hash.o:  
trs_hash.o:  
trs_hash.o:  
trs_hash.o:  
trs_hash.o:  
trs_hash.o:  
trs_hash.o:   
tree_lib.o:  
tree_lib.o:  
tree_lib.o:  
tree_lib.o:  
tree_lib.o:  
tree_lib.o:  
tree_lib.o:  
tree_lib.o:  
tree_lib.o:  
tree_lib.o:   
tree_lib.o:  
tree_lib.o:  
tree_lib.o: $(ARBHOME)/INCLUDE/arbdb.h
tree_lib.o: $(ARBHOME)/INCLUDE/ad_prot.h
tree_lib.o: $(ARBHOME)/INCLUDE/cat_tree.hxx tree_lib.hxx trs_proto.h
trs_server.o:  
trs_server.o:  
trs_server.o:  
trs_server.o:  
trs_server.o:  
trs_server.o:  
trs_server.o:  
trs_server.o:  
trs_server.o:  
trs_server.o:  
trs_server.o: $(ARBHOME)/INCLUDE/cat_tree.hxx tree_lib.hxx trs_proto.h
