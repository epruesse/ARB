# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx

# --------------------------------------------------------------------------------

LIBS = -L$(ARBHOME)/LIBLINK -lARBDB $(SYSLIBS) $(CCPLIBS)

# --------------------------------------------------------------------------------

DEST_EXECUTABLES = \
	$(ARBHOME)/bin/arb_probe_set2 \
	$(ARBHOME)/bin/arb_probe_set \

all : $(DEST_EXECUTABLES)

# --------------------------------------------------------------------------------

SHARED_OBJECTS = ps_node.o

# --------------------------------------------------------------------------------

PROBE_SET_OBJECTS = ps_dump_db.o
$(ARBHOME)/bin/arb_probe_set : $(SHARED_OBJECTS) $(PROBE_SET_OBJECTS)
	$(CPP) -o $@ $(SHARED_OBJECTS) $(PROBE_SET_OBJECTS) $(LIBS)

PROBE_SET2_OBJECTS = ps_merge_my_dbs.o
$(ARBHOME)/bin/arb_probe_set2 : $(SHARED_OBJECTS) $(PROBE_SET2_OBJECTS)
	$(CPP) -o $@ $(SHARED_OBJECTS) $(PROBE_SET2_OBJECTS) $(LIBS)

CPP_OBJECTS = $(SHARED_OBJECTS) $(PROBE_SET_OBJECTS) $(PROBE_SET2_OBJECTS)

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

ps_dump_db.o: $(ARBHOME)/INCLUDE/arbdb.h
ps_dump_db.o: $(ARBHOME)/INCLUDE/ad_prot.h ps_node.hxx
ps_dump_db.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_dump_db.o:  ../PROBE_GROUP/pg_search.hxx
ps_dump_db.o: ../PROBE_GROUP/pg_def.hxx ../PROBE_GROUP/pg_db.hxx
ps_dump_db.o: ps_pg_tree_functions.cxx
ps_merge_my_dbs.o:  ps_node.hxx
ps_merge_my_dbs.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_merge_my_dbs.o:  ../PROBE_GROUP/pg_search.hxx
ps_merge_my_dbs.o: ../PROBE_GROUP/pg_def.hxx ../PROBE_GROUP/pg_db.hxx
ps_merge_my_dbs.o: ps_pg_tree_functions.cxx
