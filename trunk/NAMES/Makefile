# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .c
OBJECTS = names.o 
$(MAIN): $(OBJECTS)
	$(AR) $(MAIN) $(OBJECTS)
.c.o: 
	$(ACC) $(cflags) -c $<  $(AINCLUDES)
.cxx.o: 
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)
depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(OBJECTS:.o=.cxx)
# DO NOT DELETE THIS LINE -- make depend depends on it.

names.o:  
names.o:  
names.o:  
names.o:  
names.o:  
names.o:  
names.o:  
names.o: $(ARBHOME)/INCLUDE/names_server.h
names.o: $(ARBHOME)/INCLUDE/names_client.h names.h
names.o: $(ARBHOME)/INCLUDE/arbdb.h
names.o: $(ARBHOME)/INCLUDE/ad_prot.h
names.o: $(ARBHOME)/INCLUDE/names_prototypes.h
names.o: $(ARBHOME)/INCLUDE/server.h
names.o: $(ARBHOME)/INCLUDE/client.h
names.o: $(ARBHOME)/INCLUDE/servercntrl.h
names.o: $(ARBHOME)/INCLUDE/struct_man.h
