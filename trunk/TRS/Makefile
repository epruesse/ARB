
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


tree_lib.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
tree_lib.c.o: $(ARBHOME)/INCLUDE/arbdb.h
tree_lib.c.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
tree_lib.c.o:  
tree_lib.c.o:  
tree_lib.c.o:  
tree_lib.c.o:  
tree_lib.c.o:  
tree_lib.c.o:  
tree_lib.c.o:  
tree_lib.c.o:   
tree_lib.c.o:   
tree_lib.c.o:  
tree_lib.c.o:  
tree_lib.c.o:  
tree_lib.c.o:  
tree_lib.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
tree_lib.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
tree_lib.c.o: tree_lib.hxx trs_proto.h
trs_hash.c.o:  
trs_hash.c.o:  
trs_hash.c.o:  
trs_hash.c.o:  
trs_hash.c.o:  
trs_hash.c.o:  
trs_hash.c.o:  
trs_hash.c.o:   
trs_hash.c.o:  
trs_hash.c.o:  
trs_hash.c.o:  
trs_hash.c.o: 
trs_hash.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
trs_hash.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
trs_server.c.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
trs_server.c.o:  
trs_server.c.o:  
trs_server.c.o:  
trs_server.c.o:  
trs_server.c.o:  
trs_server.c.o:  
trs_server.c.o:  
trs_server.c.o:  
trs_server.c.o:  
trs_server.c.o:  
trs_server.c.o:  
trs_server.c.o: 
trs_server.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
trs_server.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
trs_server.c.o: tree_lib.hxx trs_proto.h
