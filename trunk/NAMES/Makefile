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


names.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
names.c.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
names.c.o: $(ARBHOME)/INCLUDE/arbdb.h
names.c.o: $(ARBHOME)/INCLUDE/client.h
names.c.o: $(ARBHOME)/INCLUDE/names_client.h
names.c.o: $(ARBHOME)/INCLUDE/names_prototypes.h
names.c.o: $(ARBHOME)/INCLUDE/names_server.h
names.c.o: $(ARBHOME)/INCLUDE/server.h
names.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
names.c.o: $(ARBHOME)/INCLUDE/struct_man.h
names.c.o:  
names.c.o:  
names.c.o:  
names.c.o:  
names.c.o:  
names.c.o:  
names.c.o:  
names.c.o:   
names.c.o:   
names.c.o:  
names.c.o:  
names.c.o:  
names.c.o:   
names.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
names.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h names.h
