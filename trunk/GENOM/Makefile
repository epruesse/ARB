# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx


CPP_OBJECTS =   \
	GEN_read.o \
	GEN_db.o \
	GEN_interface.o \

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

clean:
	rm *.o
	rm *.a
	rm *.bak
	rm *~
	rm *#
	rm *\%

#.IGNORE:
#	clean

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)
# DO NOT DELETE THIS LINE -- make depend depends on it.

GEN_read.o: $(ARBHOME)/INCLUDE/arbdb.h 
GEN_read.o:  
GEN_read.o:  
GEN_read.o:  
GEN_read.o:  $(ARBHOME)/INCLUDE/ad_prot.h
GEN_read.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_read.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GEN_read.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GEN_read.o: $(ARBHOME)/INCLUDE/awtc_rename.hxx GEN.hxx
GEN_read.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GEN_read.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GEN_db.o: $(ARBHOME)/INCLUDE/arbdb.h 
GEN_db.o:  
GEN_db.o:  
GEN_db.o:  
GEN_db.o:  $(ARBHOME)/INCLUDE/ad_prot.h
GEN_interface.o: $(ARBHOME)/INCLUDE/awt.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/arbdb.h 
GEN_interface.o:  
GEN_interface.o:  
GEN_interface.o:  
GEN_interface.o: 
GEN_interface.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_interface.o: $(ARBHOME)/INCLUDE/ntree.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_interface.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GEN_interface.o: $(ARBHOME)/INCLUDE/probe_design.hxx GEN_local.hxx
GEN_interface.o: GEN.hxx
