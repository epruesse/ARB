# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .c .cxx
OBJECTS = db_server.o

$(MAIN): $(OBJECTS)
	$(AR) $(MAIN) $(OBJECTS)
$(OBJECTS): $(LOCALS) $(ARBDBH) $(SERVERCNTRLH)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(OBJECTS:.o=.cxx)

.cxx.o: 
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

# DO NOT DELETE THIS LINE -- make depend depends on it.

db_server.o:  
db_server.o:  
db_server.o:  
db_server.o:  
db_server.o:  $(ARBHOME)/INCLUDE/arbdb.h
db_server.o: $(ARBHOME)/INCLUDE/ad_prot.h
db_server.o: $(ARBHOME)/INCLUDE/servercntrl.h
