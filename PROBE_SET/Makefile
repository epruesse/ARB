# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx

# --------------------------------------------------------------------------------

LIBS = -L$(ARBHOME)/LIBLINK -lARBDB $(SYSLIBS) $(CCPLIBS) 

# --------------------------------------------------------------------------------

DEST_EXECUTABLES = \
	$(ARBHOME)/bin/arb_probe_set \
	$(ARBHOME)/bin/arb_probe_set2 \

all : $(DEST_EXECUTABLES)

# --------------------------------------------------------------------------------

SHARED_OBJECTS = # z.B.ps_node.o

# --------------------------------------------------------------------------------

PROBE_SET_OBJECTS = ps_dump_db.o
$(ARBHOME)/bin/arb_probe_set : $(SHARED_OBJECTS) $(PROBE_SET_OBJECTS)
	$(CPP) -o $@ $< $(LIBS)

PROBE_SET2_OBJECTS = ps_dump_db.o
$(ARBHOME)/bin/arb_probe_set2 : $(SHARED_OBJECTS) $(PROBE_SET2_OBJECTS)
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
