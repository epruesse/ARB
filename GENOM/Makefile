# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx


CPP_OBJECTS =   \
	GEN_read.o \
	GEN_db.o \

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


GEN_db.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_db.c.o: $(ARBHOME)/INCLUDE/arbdb.h
GEN_db.c.o:  
GEN_db.c.o:  
GEN_db.c.o:  
GEN_db.c.o:   
GEN_db.c.o: 
GEN_db.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
GEN_db.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
GEN_db.c.o: GEN_db.hxx
GEN_read.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_read.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GEN_read.c.o: $(ARBHOME)/INCLUDE/arbdb.h
GEN_read.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_read.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GEN_read.c.o: $(ARBHOME)/INCLUDE/awtc_rename.hxx
GEN_read.c.o:  
GEN_read.c.o:  
GEN_read.c.o:  
GEN_read.c.o:  
GEN_read.c.o:  
GEN_read.c.o:  
GEN_read.c.o:  
GEN_read.c.o:  
GEN_read.c.o: /usr/include/g++/cctype /usr/include/g++/cstddef
GEN_read.c.o: /usr/include/g++/cstring 
GEN_read.c.o: /usr/include/g++/iterator /usr/include/g++/std/bastring.cc
GEN_read.c.o:  
GEN_read.c.o:  
GEN_read.c.o:  
GEN_read.c.o:  /usr/include/g++/string
GEN_read.c.o:  
GEN_read.c.o:  
GEN_read.c.o:   
GEN_read.c.o:  
GEN_read.c.o:  
GEN_read.c.o:  
GEN_read.c.o: 
GEN_read.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
GEN_read.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h GEN.hxx
GEN_read.c.o: GEN_db.hxx
