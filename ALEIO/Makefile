LIB = -L../LIBLINK -lARBDO $(SYSLIBS)

OBJECT = io_arbdb.o getopt.o getopt1.o xmalloc.o lenstring.o careful.o

.SUFFIXES: .o .c .cxx

all: $(ARBHOME)/bin/arb_aleio

$(ARBHOME)/bin/arb_aleio:  $(OBJECT)
	$(ACC) $(cflags) -o $@ $(OBJECT)  $(LIB)

.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(OBJECT:.o=.c*)

# DO NOT DELETE THIS LINE -- make depend depends on it.

io_arbdb.o:  
io_arbdb.o:  
io_arbdb.o:  
io_arbdb.o:  
io_arbdb.o:  $(ARBHOME)/INCLUDE/arbdb.h
io_arbdb.o: $(ARBHOME)/INCLUDE/ad_prot.h
io_arbdb.o: $(ARBHOME)/INCLUDE/arbdbt.h
io_arbdb.o: $(ARBHOME)/INCLUDE/ad_t_prot.h ring.h getopt.h
io_arbdb.o: xmalloc.h  
io_arbdb.o:  
io_arbdb.o:  
io_arbdb.o:  
io_arbdb.o:   lenstring.h
io_arbdb.o: hash.h careful.h
getopt.o:  
getopt.o:  
getopt.o:  
getopt.o:  
getopt.o: 
getopt1.o: getopt.h  
getopt1.o:  
getopt1.o:  
getopt1.o:  
xmalloc.o:  
xmalloc.o:  
xmalloc.o:  
xmalloc.o:  
xmalloc.o:  
xmalloc.o:  
xmalloc.o:  
xmalloc.o:   
xmalloc.o:  xmalloc.h
lenstring.o:  
lenstring.o:  
lenstring.o:  
lenstring.o:  
lenstring.o:  
lenstring.o:  
lenstring.o:  
lenstring.o:  
lenstring.o:  
lenstring.o:  lenstring.h xmalloc.h
careful.o:  
careful.o:  
careful.o:  
careful.o:  
careful.o:  
careful.o:  
careful.o: 
