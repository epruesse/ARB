# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx

CPP_OBJECTS = WETC_main.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

.cxx.o: 
	$(CPP) $(cflags) -c -o $@ $(@:.o=.cxx)  $(CPPINCLUDES)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)


# DO NOT DELETE THIS LINE -- make depend depends on it.


WETC_main.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
WETC_main.c.o: $(ARBHOME)/INCLUDE/arbdb.h
WETC_main.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
WETC_main.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
WETC_main.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
WETC_main.c.o: $(ARBHOME)/INCLUDE/awt.hxx
WETC_main.c.o:  
WETC_main.c.o:  
WETC_main.c.o:  
WETC_main.c.o:  
WETC_main.c.o:  
WETC_main.c.o:  
WETC_main.c.o:   
WETC_main.c.o:  
WETC_main.c.o:  
WETC_main.c.o:  
WETC_main.c.o: 
WETC_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
WETC_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
