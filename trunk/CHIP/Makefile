# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx

# --------------------------------------------------------------------------------

# --------------------------------------------------------------------------------

CPP_OBJECTS = probe_functions.o \
		chip_main.o

all :
	$(CLEAN_BEFORE_MAKE)
	$(MAKE) $(MAIN)

$(MAIN): $(CPP_OBJECTS)
	$(XAR) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c -o $@ $(@:.o=.cxx)  $(CPPINCLUDES)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)

clean:
	-rm *.a *.o
	-rm -r SunWS_cache

# DO NOT DELETE

probe_functions.o: $(ARBHOME)/INCLUDE/arbdb.h
probe_functions.o: $(ARBHOME)/INCLUDE/arb_assert.h
probe_functions.o: $(ARBHOME)/INCLUDE/ad_prot.h
probe_functions.o: $(ARBHOME)/INCLUDE/aw_window.hxx
probe_functions.o: $(ARBHOME)/INCLUDE/aw_root.hxx
probe_functions.o: $(ARBHOME)/INCLUDE/aw_device.hxx
probe_functions.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
probe_functions.o: $(ARBHOME)/INCLUDE/PT_com.h
probe_functions.o: $(ARBHOME)/INCLUDE/client.h
probe_functions.o: $(ARBHOME)/INCLUDE/servercntrl.h
probe_functions.o: probe_functions.hxx chip.hxx
