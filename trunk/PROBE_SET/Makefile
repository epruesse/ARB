# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx

# --------------------------------------------------------------------------------

LIBS = -L$(ARBHOME)/LIBLINK -lARBDB $(SYSLIBS) $(CCPLIBS)

# --------------------------------------------------------------------------------

DEST_EXECUTABLES = \
	$(ARBHOME)/PROBE_SET/bin/ps_show_result \
	$(ARBHOME)/PROBE_SET/bin/ps_arb2asciipaths \
	$(ARBHOME)/PROBE_SET/bin/ps_my2asciipaths \
	$(ARBHOME)/PROBE_SET/bin/ps_my2ascii \
	$(ARBHOME)/PROBE_SET/bin/ps_find \
	$(ARBHOME)/PROBE_SET/bin/ps_detect \
	$(ARBHOME)/PROBE_SET/bin/ps_merge \
	$(ARBHOME)/PROBE_SET/bin/ps_convert \
	$(ARBHOME)/PROBE_SET/bin/fb_test \

all : $(DEST_EXECUTABLES)

# --------------------------------------------------------------------------------

SHARED_OBJECTS = ps_node.o ps_filebuffer.o ps_database.o

# --------------------------------------------------------------------------------

PROBE_SET_TEST_OBJECTS = fb_test.o
$(ARBHOME)/PROBE_SET/bin/fb_test : $(SHARED_OBJECTS) $(PROBE_SET_TEST_OBJECTS)
	$(CPP) -o $@ $(SHARED_OBJECTS) $(PROBE_SET_TEST_OBJECTS) $(LIBS)
#	$(CPP) -pg -g -a -o $@ $(SHARED_OBJECTS) $(PROBE_SET_TEST_OBJECTS) $(LIBS)

PROBE_SET_CORE1_OBJECTS = ps_convert_db.o
$(ARBHOME)/PROBE_SET/bin/ps_convert : $(SHARED_OBJECTS) $(PROBE_SET_CORE1_OBJECTS)
	$(CPP) -o $@ $(SHARED_OBJECTS) $(PROBE_SET_CORE1_OBJECTS) $(LIBS)
#	$(CPP) -pg -g -a -o $@ $(SHARED_OBJECTS) $(PROBE_SET_CORE1_OBJECTS) $(LIBS)

PROBE_SET_CORE2_OBJECTS = ps_merge_my_dbs.o
$(ARBHOME)/PROBE_SET/bin/ps_merge : $(SHARED_OBJECTS) $(PROBE_SET_CORE2_OBJECTS)
	$(CPP) -o $@ $(SHARED_OBJECTS) $(PROBE_SET_CORE2_OBJECTS) $(LIBS)
#	$(CPP) -pg -g -a -o $@ $(SHARED_OBJECTS) $(PROBE_SET_CORE2_OBJECTS) $(LIBS)

PROBE_SET_CORE3_OBJECTS = ps_detect_weak_differences.o
$(ARBHOME)/PROBE_SET/bin/ps_detect : $(SHARED_OBJECTS) $(PROBE_SET_CORE3_OBJECTS)
	$(CPP) -o $@ $(SHARED_OBJECTS) $(PROBE_SET_CORE3_OBJECTS) $(LIBS)
#	$(CPP) -pg -g -a -o $@ $(SHARED_OBJECTS) $(PROBE_SET_CORE3_OBJECTS) $(LIBS)

PROBE_SET_CORE4_OBJECTS = ps_find_probes.o
$(ARBHOME)/PROBE_SET/bin/ps_find : $(SHARED_OBJECTS) $(PROBE_SET_CORE3_OBJECTS)
	$(CPP) -o $@ $(SHARED_OBJECTS) $(PROBE_SET_CORE3_OBJECTS) $(LIBS)
#	$(CPP) -pg -g -a -o $@ $(SHARED_OBJECTS) $(PROBE_SET_CORE3_OBJECTS) $(LIBS)

PROBE_SET_AUX1_OBJECTS = ps_my2ascii.o
$(ARBHOME)/PROBE_SET/bin/ps_my2ascii : $(SHARED_OBJECTS) $(PROBE_SET_AUX1_OBJECTS)
	$(CPP) -o $@ $(SHARED_OBJECTS) $(PROBE_SET_AUX1_OBJECTS) $(LIBS)
#	$(CPP) -pg -g -a -o $@ $(SHARED_OBJECTS) $(PROBE_SET_AUX1_OBJECTS) $(LIBS)

PROBE_SET_AUX2_OBJECTS = ps_my2asciipaths.o
$(ARBHOME)/PROBE_SET/bin/ps_my2asciipaths : $(SHARED_OBJECTS) $(PROBE_SET_AUX2_OBJECTS)
	$(CPP) -o $@ $(SHARED_OBJECTS) $(PROBE_SET_AUX2_OBJECTS) $(LIBS)
#	$(CPP) -pg -g -a -o $@ $(SHARED_OBJECTS) $(PROBE_SET_AUX2_OBJECTS) $(LIBS)

PROBE_SET_AUX3_OBJECTS = ps_arb2asciipaths.o
$(ARBHOME)/PROBE_SET/bin/ps_arb2asciipaths : $(SHARED_OBJECTS) $(PROBE_SET_AUX3_OBJECTS)
	$(CPP) -o $@ $(SHARED_OBJECTS) $(PROBE_SET_AUX3_OBJECTS) $(LIBS)
#	$(CPP) -pg -g -a -o $@ $(SHARED_OBJECTS) $(PROBE_SET_AUX3_OBJECTS) $(LIBS)

PROBE_SET_AUX4_OBJECTS = ps_show_result.o
$(ARBHOME)/PROBE_SET/bin/ps_show_result : $(SHARED_OBJECTS) $(PROBE_SET_AUX4_OBJECTS)
	$(CPP) -o $@ $(SHARED_OBJECTS) $(PROBE_SET_AUX4_OBJECTS) $(LIBS)
#	$(CPP) -pg -g -a -o $@ $(SHARED_OBJECTS) $(PROBE_SET_AUX4_OBJECTS) $(LIBS)

CPP_OBJECTS = $(SHARED_OBJECTS) \
	$(PROBE_SET_TEST_OBJECTS) \
	$(PROBE_SET_CORE1_OBJECTS) \
	$(PROBE_SET_CORE2_OBJECTS) \
	$(PROBE_SET_CORE3_OBJECTS) \
	$(PROBE_SET_CORE4_OBJECTS) \
	$(PROBE_SET_AUX1_OBJECTS) \
	$(PROBE_SET_AUX2_OBJECTS) \
	$(PROBE_SET_AUX3_OBJECTS) \
	$(PROBE_SET_AUX4_OBJECTS) \

.cxx.o:
	$(CPP) $(cflags) -c -o $@ $(@:.o=.cxx)  $(CPPINCLUDES)
#	$(CPP) $(cflags) -pg -g -a -c -o $@ $(@:.o=.cxx)  $(CPPINCLUDES)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)

clean:
	-rm *.a *.o
	-rm -r SunWS_cache
	-rm $(DEST_EXECUTABLES)

# DO NOT DELETE

ps_node.o: ps_node.hxx $(ARBHOME)/INCLUDE/smartptr.h
ps_node.o: $(ARBHOME)/INCLUDE/arb_assert.h ps_defs.hxx
ps_node.o: ps_filebuffer.hxx  
ps_filebuffer.o: ps_filebuffer.hxx 
ps_database.o: ps_database.hxx ps_defs.hxx ps_node.hxx
ps_database.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_database.o: $(ARBHOME)/INCLUDE/arb_assert.h ps_filebuffer.hxx
fb_test.o: ps_bitmap.hxx ps_bitset.hxx ps_filebuffer.hxx 
fb_test.o:  ps_defs.hxx ps_node.hxx
fb_test.o: $(ARBHOME)/INCLUDE/smartptr.h
fb_test.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_convert_db.o: ps_database.hxx ps_defs.hxx ps_node.hxx
ps_convert_db.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_convert_db.o: $(ARBHOME)/INCLUDE/arb_assert.h ps_filebuffer.hxx
ps_convert_db.o: ps_pg_tree_functions.cxx
ps_convert_db.o: $(ARBHOME)/INCLUDE/arbdb.h
ps_convert_db.o: $(ARBHOME)/INCLUDE/ad_prot.h
ps_merge_my_dbs.o:  ps_database.hxx ps_defs.hxx
ps_merge_my_dbs.o: ps_node.hxx $(ARBHOME)/INCLUDE/smartptr.h
ps_merge_my_dbs.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_merge_my_dbs.o: ps_filebuffer.hxx 
ps_detect_weak_differences.o: ps_database.hxx ps_defs.hxx ps_node.hxx
ps_detect_weak_differences.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_detect_weak_differences.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_detect_weak_differences.o: ps_filebuffer.hxx 
ps_detect_weak_differences.o:  ps_bitmap.hxx
ps_detect_weak_differences.o: ps_bitset.hxx
ps_find_probes.o:  ps_database.hxx ps_defs.hxx
ps_find_probes.o: ps_node.hxx $(ARBHOME)/INCLUDE/smartptr.h
ps_find_probes.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_find_probes.o: ps_filebuffer.hxx 
ps_find_probes.o: ps_bitmap.hxx ps_bitset.hxx
ps_my2ascii.o:  ps_filebuffer.hxx 
ps_my2ascii.o: ps_node.hxx $(ARBHOME)/INCLUDE/smartptr.h
ps_my2ascii.o: $(ARBHOME)/INCLUDE/arb_assert.h ps_defs.hxx
ps_my2asciipaths.o:  ps_defs.hxx ps_node.hxx
ps_my2asciipaths.o: $(ARBHOME)/INCLUDE/smartptr.h
ps_my2asciipaths.o: $(ARBHOME)/INCLUDE/arb_assert.h
ps_my2asciipaths.o: ps_filebuffer.hxx 
ps_arb2asciipaths.o: $(ARBHOME)/INCLUDE/arbdb.h
ps_arb2asciipaths.o: $(ARBHOME)/INCLUDE/ad_prot.h ps_defs.hxx
ps_arb2asciipaths.o: ps_filebuffer.hxx 
ps_arb2asciipaths.o:  ps_pg_tree_functions.cxx
ps_show_result.o: ps_defs.hxx ps_filebuffer.hxx 
ps_show_result.o:  ps_bitmap.hxx ps_bitset.hxx
