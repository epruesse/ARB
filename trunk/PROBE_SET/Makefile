# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx

# --------------------------------------------------------------------------------

LIBS = -L$(ARBHOME)/LIBLINK -lARBDB $(SYSLIBS) $(CCPLIBS)

# --------------------------------------------------------------------------------

DEST_EXECUTABLES = \
	$(ARBHOME)/bin/arb_probe_set3 \
	$(ARBHOME)/bin/arb_probe_set2 \
	$(ARBHOME)/bin/arb_probe_set1 \

all : $(DEST_EXECUTABLES)

# --------------------------------------------------------------------------------

SHARED_OBJECTS = ps_node.o

# --------------------------------------------------------------------------------

PROBE_SET1_OBJECTS = ps_dump_db.o
$(ARBHOME)/bin/arb_probe_set1 : $(SHARED_OBJECTS) $(PROBE_SET1_OBJECTS)
	$(CPP) -o $@ $(SHARED_OBJECTS) $(PROBE_SET1_OBJECTS) $(LIBS)

PROBE_SET2_OBJECTS = ps_merge_my_dbs.o
$(ARBHOME)/bin/arb_probe_set2 : $(SHARED_OBJECTS) $(PROBE_SET2_OBJECTS)
	$(CPP) -o $@ $(SHARED_OBJECTS) $(PROBE_SET2_OBJECTS) $(LIBS)

PROBE_SET3_OBJECTS = ps_make_sample_db.o
$(ARBHOME)/bin/arb_probe_set3 : $(SHARED_OBJECTS) $(PROBE_SET3_OBJECTS)
	$(CPP) -o $@ $(SHARED_OBJECTS) $(PROBE_SET3_OBJECTS) $(LIBS)

CPP_OBJECTS = $(SHARED_OBJECTS) $(PROBE_SET1_OBJECTS) $(PROBE_SET2_OBJECTS) $(PROBE_SET3_OBJECTS)

#	$(CLEAN_BEFORE_MAKE)
#	$(MAKE) $(ARBHOME)/bin/arb_probe_set
#	$(MAKE) $(ARBHOME)/bin/arb_probe_blub
#	$(MAKE) $(ARBHOME)/bin/arb_probe_bla

.cxx.o:
	$(CPP) $(cflags) -c -o $@ $(@:.o=.cxx)  $(CPPINCLUDES)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)

clean:
	-rm *.a *.o
	-rm -r SunWS_cache
	-rm $(DEST_EXECUTABLES)

# DO NOT DELETE

ps_node.o: ps_node.hxx $(ARBHOME)/INCLUDE/smartptr.h
ps_node.o:  ../PROBE_GROUP/pg_search.hxx
ps_node.o: ../PROBE_GROUP/pg_def.hxx ../PROBE_GROUP/pg_db.hxx
ps_node.o: $(ARBHOME)/INCLUDE/arbdb.h 
ps_node.o: $(ARBHOME)/INCLUDE/ad_prot.h
ps_dump_db.o:  $(ARBHOME)/INCLUDE/arbdb.h
ps_dump_db.o: $(ARBHOME)/INCLUDE/ad_prot.h ps_node.hxx
ps_dump_db.o: $(ARBHOME)/INCLUDE/smartptr.h 
ps_dump_db.o: ../PROBE_GROUP/pg_search.hxx ../PROBE_GROUP/pg_def.hxx
ps_dump_db.o: ../PROBE_GROUP/pg_db.hxx ps_pg_tree_functions.cxx
ps_merge_my_dbs.o:  ps_node.hxx
ps_merge_my_dbs.o:  ps_bitmap.hxx
ps_merge_my_dbs.o:  ps_bitset.hxx
ps_merge_my_dbs.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_merge_my_dbs.o:  ../PROBE_GROUP/pg_search.hxx
ps_merge_my_dbs.o: ../PROBE_GROUP/pg_def.hxx ../PROBE_GROUP/pg_db.hxx
ps_merge_my_dbs.o: $(ARBHOME)/INCLUDE/arbdb.h
ps_merge_my_dbs.o: $(ARBHOME)/INCLUDE/ad_prot.h ps_bitset.hxx
ps_make_sample_db.o:  ps_node.hxx
ps_make_sample_db.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_make_sample_db.o: ../PROBE_GROUP/pg_search.hxx ../PROBE_GROUP/pg_def.hxx
ps_make_sample_db.o: ../PROBE_GROUP/pg_db.hxx
ps_make_sample_db.o: $(ARBHOME)/INCLUDE/arbdb.h
ps_make_sample_db.o: $(ARBHOME)/INCLUDE/ad_prot.h
