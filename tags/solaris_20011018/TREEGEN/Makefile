OBJECTS = base.o defines.o frand.o readcfg.o rns.o sim.o simcfg.o spreadin.o

tg: $(OBJECTS)
	acc -o $@ $(OBJECTS)

.c.o:
	acc -g -c -o $@ $<
