# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .c .cxx .o .h

CPP_OBJECTS =
C_OBJECTS = base.o defines.o frand.o readcfg.o rns.o sim.o simcfg.o spreadin.o

$(MAIN): $(CPP_OBJECTS) $(C_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS) $(C_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

.c.o:
	$(ACC) $(cflags) -c $< $(CPPINCLUDES)

clean:
	@-rm $(CPP_OBJECTS) $(C_OBJECTS)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx) $(C_OBJECTS:.o=.c)


#tg: $(OBJECTS)
#	gcc -o $@ $(OBJECTS)

#.c.o:
#	gcc -g -c -o $@ $<
# DO NOT DELETE

base.o: base.h
defines.o: defines.h
frand.o: frand.h
readcfg.o: readcfg.h
rns.o: rns.h spreadin.h  
simcfg.o: simcfg.h
spreadin.o: spreadin.h
