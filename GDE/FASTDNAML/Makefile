.SUFFIXES: .o .c

OBJECT = fastDNAml_1_2_1.o

$(ARBHOME)/bin/fastdnaml: $(OBJECT)
	$(ACC) $(cflags) -o $@ $< $(LIB) -lm

.c.o:
	$(ACC) $(cflags) -c -o $@ $< $(AINCLUDES)

depend:
	makedepend $(MAKEDEPENDINC) $(OBJECT:.o=.c)

# DO NOT DELETE

fastDNAml_1_2_1.o:  fastDNAml_1_2_1.h
