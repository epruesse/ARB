# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .c .cxx

OBJECTS = TC_main.o BI_helix.o

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)
.cxx.o: 
	$(CPP) $(cflags) -c -o $@ $< $(CPPINCLUDES)
depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(OBJECTS:.o=.cxx)
# DO NOT DELETE THIS LINE -- make depend depends on it.

TC_main.o:
TC_main.o:
TC_main.o: $(ARBHOME)/INCLUDE/arbdb.h
TC_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
TC_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
TC_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h tc.hxx
TC_main.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
BI_helix.o:
BI_helix.o:
BI_helix.o:
BI_helix.o: $(ARBHOME)/INCLUDE/arbdb.h
BI_helix.o: $(ARBHOME)/INCLUDE/ad_prot.h
BI_helix.o: $(ARBHOME)/INCLUDE/arbdbt.h
BI_helix.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
BI_helix.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
