# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx

# --------------------------------------------------------------------------------

LIBS = -L$(ARBHOME)/LIBLINK -lARBDB $(SYSLIBS) $(CCPLIBS) 

# --------------------------------------------------------------------------------

DEST_EXECUTABLES = \
	$(ARBHOME)/bin/arb_probe_set \

#	$(ARBHOME)/bin/arb_probe_set2 \

all : $(DEST_EXECUTABLES)

# --------------------------------------------------------------------------------

SHARED_OBJECTS = # z.B.ps_node.o

# --------------------------------------------------------------------------------

PROBE_SET_OBJECTS = ps_dump_db.o
$(ARBHOME)/bin/arb_probe_set : $(SHARED_OBJECTS) $(PROBE_SET_OBJECTS)
	$(CPP) -o $@ $< $(LIBS)

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
ps_dump_db.o: $(ARBHOME)/INCLUDE/ad_prot.h pg_search.hxx
ps_dump_db.o: pg_def.hxx pg_db.hxx
ps_dump_db.o: $(ARBHOME)/INCLUDE/smartptr.h
