# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx

# --------------------------------------------------------------------------------

LIBS = -L$(ARBHOME)/LIBLINK -lARBDB $(SYSLIBS) $(CCPLIBS)

# --------------------------------------------------------------------------------

DEST_EXECUTABLES = \
	$(ARBHOME)/PROBE_SET/bin/fb_test \
	$(ARBHOME)/PROBE_SET/bin/ps_my2ascii \
	$(ARBHOME)/bin/arb_probe_set4 \
	$(ARBHOME)/bin/arb_probe_set3 \
	$(ARBHOME)/bin/arb_probe_set2 \
	$(ARBHOME)/bin/arb_probe_set1 \

ENABLE_PROFILING = 0
ifeq ($(ENABLE_PROFILING),1)
PROFILE_OPTS = -pg -g
else
PROFILE_OPTS =
endif

all : $(DEST_EXECUTABLES)

# --------------------------------------------------------------------------------

SHARED_OBJECTS = ps_node.o ps_filebuffer.o

# --------------------------------------------------------------------------------

PROBE_SET1_OBJECTS = ps_dump_db.o
$(ARBHOME)/bin/arb_probe_set1 : $(SHARED_OBJECTS) $(PROBE_SET1_OBJECTS)
	$(CPP) $(PROFILE_OPTS) -o $@ $(SHARED_OBJECTS) $(PROBE_SET1_OBJECTS) $(LIBS)
	-@ln -sf ../../bin/arb_probe_set1 bin/ps_convert

PROBE_SET2_OBJECTS = ps_merge_my_dbs.o
$(ARBHOME)/bin/arb_probe_set2 : $(SHARED_OBJECTS) $(PROBE_SET2_OBJECTS)
	$(CPP) $(PROFILE_OPTS) -o $@ $(SHARED_OBJECTS) $(PROBE_SET2_OBJECTS) $(LIBS)
	-@ln -sf ../../bin/arb_probe_set2 bin/ps_merge

PROBE_SET3_OBJECTS = ps_make_sample_db.o
$(ARBHOME)/bin/arb_probe_set3 : $(SHARED_OBJECTS) $(PROBE_SET3_OBJECTS)
	$(CPP) $(PROFILE_OPTS) -o $@ $(SHARED_OBJECTS) $(PROBE_SET3_OBJECTS) $(LIBS)
	-@ln -sf ../../bin/arb_probe_set3 bin/ps_make_sample

PROBE_SET4_OBJECTS = ps_detect_weak_differences.o
$(ARBHOME)/bin/arb_probe_set4 : $(SHARED_OBJECTS) $(PROBE_SET4_OBJECTS)
	$(CPP) $(PROFILE_OPTS) -o $@ $(SHARED_OBJECTS) $(PROBE_SET4_OBJECTS) $(LIBS)
	-@ln -sf ../../bin/arb_probe_set4 bin/ps_detect

PROBE_SET5_OBJECTS = fb_test.o
$(ARBHOME)/PROBE_SET/bin/fb_test : $(SHARED_OBJECTS) $(PROBE_SET5_OBJECTS)
	$(CPP) $(PROFILE_OPTS) -o $@ $(SHARED_OBJECTS) $(PROBE_SET5_OBJECTS) $(LIBS)

PROBE_SET6_OBJECTS = ps_my2ascii.o
$(ARBHOME)/PROBE_SET/bin/ps_my2ascii : $(SHARED_OBJECTS) $(PROBE_SET6_OBJECTS)
	$(CPP) $(PROFILE_OPTS) -o $@ $(SHARED_OBJECTS) $(PROBE_SET6_OBJECTS) $(LIBS)

CPP_OBJECTS = $(SHARED_OBJECTS) \
	$(PROBE_SET1_OBJECTS) \
	$(PROBE_SET2_OBJECTS) \
	$(PROBE_SET3_OBJECTS) \
	$(PROBE_SET4_OBJECTS) \
	$(PROBE_SET5_OBJECTS) \
	$(PROBE_SET6_OBJECTS) \

.cxx.o:
	$(CPP) $(cflags) $(PROFILE_OPTS) -c -o $@ $(@:.o=.cxx)  $(CPPINCLUDES)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)

clean:
# touch to supress error messages on nonexisting files
	@touch bla.o bla.a SunWS_cache $(DEST_EXECUTABLES)
	-@rm *.a *.o $(DEST_EXECUTABLES)
	-@rm -r SunWS_cache

sex:
	@echo "sorry, but its 'have sex', not 'make sex' .. aborting"
love:
	@echo "make: *** No rule on how to make 'love'.  Stop."
war:
	@echo "make: *** No idea how to make war. War is not logical.  Stop."

# DO NOT DELETE

ps_node.o: ps_node.hxx $(ARBHOME)/INCLUDE/smartptr.h
ps_node.o: ../PROBE_GROUP/pg_search.hxx ../PROBE_GROUP/pg_def.hxx
ps_node.o: $(ARBHOME)/INCLUDE/arb_assert.h ../PROBE_GROUP/pg_db.hxx
ps_node.o: $(ARBHOME)/INCLUDE/arbdb.h 
ps_node.o:  $(ARBHOME)/INCLUDE/ad_prot.h
ps_node.o: ps_filebuffer.hxx  
ps_filebuffer.o: ps_filebuffer.hxx 
ps_dump_db.o:  $(ARBHOME)/INCLUDE/arbdb.h
ps_dump_db.o: $(ARBHOME)/INCLUDE/ad_prot.h ps_node.hxx
ps_dump_db.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_dump_db.o: ../PROBE_GROUP/pg_search.hxx ../PROBE_GROUP/pg_def.hxx
ps_dump_db.o: $(ARBHOME)/INCLUDE/arb_assert.h ../PROBE_GROUP/pg_db.hxx
ps_dump_db.o: ps_filebuffer.hxx 
ps_dump_db.o: ps_pg_tree_functions.cxx
ps_merge_my_dbs.o:  ps_filebuffer.hxx
ps_merge_my_dbs.o:  ps_node.hxx
ps_merge_my_dbs.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_merge_my_dbs.o: ../PROBE_GROUP/pg_search.hxx ../PROBE_GROUP/pg_def.hxx
ps_merge_my_dbs.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_merge_my_dbs.o: ../PROBE_GROUP/pg_db.hxx $(ARBHOME)/INCLUDE/arbdb.h
ps_merge_my_dbs.o: $(ARBHOME)/INCLUDE/ad_prot.h
ps_make_sample_db.o: ps_node.hxx $(ARBHOME)/INCLUDE/smartptr.h
ps_make_sample_db.o: ../PROBE_GROUP/pg_search.hxx ../PROBE_GROUP/pg_def.hxx
ps_make_sample_db.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_make_sample_db.o: ../PROBE_GROUP/pg_db.hxx
ps_make_sample_db.o: $(ARBHOME)/INCLUDE/arbdb.h
ps_make_sample_db.o: $(ARBHOME)/INCLUDE/ad_prot.h ps_filebuffer.hxx
ps_detect_weak_differences.o: ps_node.hxx
ps_detect_weak_differences.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_detect_weak_differences.o: ../PROBE_GROUP/pg_search.hxx
ps_detect_weak_differences.o: ../PROBE_GROUP/pg_def.hxx
ps_detect_weak_differences.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_detect_weak_differences.o: ../PROBE_GROUP/pg_db.hxx
ps_detect_weak_differences.o: $(ARBHOME)/INCLUDE/arbdb.h
ps_detect_weak_differences.o: $(ARBHOME)/INCLUDE/ad_prot.h
ps_detect_weak_differences.o: ps_filebuffer.hxx 
ps_detect_weak_differences.o:  ps_bitset.hxx
ps_detect_weak_differences.o: ps_bitmap.hxx
fb_test.o: ps_filebuffer.hxx  
ps_my2ascii.o:  ps_filebuffer.hxx 
ps_my2ascii.o: ps_speciesmap.hxx ps_node.hxx
ps_my2ascii.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_my2ascii.o: ../PROBE_GROUP/pg_search.hxx ../PROBE_GROUP/pg_def.hxx
ps_my2ascii.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_my2ascii.o: ../PROBE_GROUP/pg_db.hxx $(ARBHOME)/INCLUDE/arbdb.h
ps_my2ascii.o: $(ARBHOME)/INCLUDE/ad_prot.h
