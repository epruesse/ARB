# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c
OBJECTS = aisc.o aisc_commands.o aisc_var_ref.o aisc_mix.o

$(MAIN): aisc

aisc:	$(OBJECTS)
	$(CPP) $(cflags) -o $@ $(OBJECTS)

.c.o:
	$(CPP) $(cflags) -c $< $(AINCLUDES)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(OBJECTS:.o=.c)

clean:
	rm -f *.[ao]
	rm -f $(OBJECTS:.o=)
	rm -f *%
	rm -f core
	mv doc/doc.pub .
	rm -f doc/*
	mv doc.pub doc

proto:
	rm -f aisc_proto.h
	../AISC_MKPTPS/aisc_mkpt -C *.c >aisc_proto.h

# DO NOT DELETE THIS LINE -- make depend depends on it.

aisc.o:  aisc.h aisc_proto.h
aisc_commands.o:  aisc.h aisc_proto.h
aisc_var_ref.o:  aisc.h aisc_proto.h
aisc_mix.o:  aisc.h aisc_proto.h
