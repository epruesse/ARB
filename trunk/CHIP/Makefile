#-----------------------------------------------------------------------------
# Makefile for ca_mark and ca_probe
# Kai Bader (baderk@in.tum.de)
# 2003, TU Muenchen
#-----------------------------------------------------------------------------


LIB = $(LIBPATH) $(SYSLIBS) $(CCPLIBS) -lARBDB
#LIB = $(LIBPATH) $(SYSLIBS) $(CCPLIBS) -L /usr/arb/LIBLINK -lARBDB

.SUFFIXES: .o .c .cxx

OBJECT = ca_mark.o ca_probe.o

all: $(OBJECT:%.o=$(ARBHOME)/bin/%)

$(ARBHOME)/bin/ca_probe: ca_probe.cxx
	$(CPP) $(cflags) -ggdb  -o $@ $< $(CPPINCLUDES) ../SERVERCNTRL/SERVERCNTRL.a ../PROBE_COM/client.a $(LIB)

$(ARBHOME)/bin/%: %.cxx
	$(CPP) $(cflags) -ggdb -o $@ $< $(CPPINCLUDES) $(LIB)

# DO NOT DELETE

$(ARBHOME)/bin/ca_mark: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/ca_mark: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/ca_mark: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/ca_mark: $(ARBHOME)/INCLUDE/aw_window.hxx
$(ARBHOME)/bin/ca_mark: $(ARBHOME)/INCLUDE/aw_root.hxx
$(ARBHOME)/bin/ca_mark: $(ARBHOME)/INCLUDE/aw_device.hxx
$(ARBHOME)/bin/ca_mark: $(ARBHOME)/INCLUDE/aw_awars.hxx
$(ARBHOME)/bin/ca_mark: $(ARBHOME)/INCLUDE/PT_com.h
$(ARBHOME)/bin/ca_mark: $(ARBHOME)/INCLUDE/client.h
$(ARBHOME)/bin/ca_mark: $(ARBHOME)/INCLUDE/servercntrl.h
$(ARBHOME)/bin/ca_mark: ca_mark.hxx

$(ARBHOME)/bin/ca_probe: $(ARBHOME)/INCLUDE/arbdb.h
$(ARBHOME)/bin/ca_probe: $(ARBHOME)/INCLUDE/arb_assert.h
$(ARBHOME)/bin/ca_probe: $(ARBHOME)/INCLUDE/ad_prot.h
$(ARBHOME)/bin/ca_probe: $(ARBHOME)/INCLUDE/aw_window.hxx
$(ARBHOME)/bin/ca_probe: $(ARBHOME)/INCLUDE/aw_root.hxx
$(ARBHOME)/bin/ca_probe: $(ARBHOME)/INCLUDE/aw_device.hxx
$(ARBHOME)/bin/ca_probe: $(ARBHOME)/INCLUDE/aw_awars.hxx
$(ARBHOME)/bin/ca_probe: $(ARBHOME)/INCLUDE/PT_com.h
$(ARBHOME)/bin/ca_probe: $(ARBHOME)/INCLUDE/client.h
$(ARBHOME)/bin/ca_probe: $(ARBHOME)/INCLUDE/servercntrl.h
$(ARBHOME)/bin/ca_probe: ca_probe.hxx
