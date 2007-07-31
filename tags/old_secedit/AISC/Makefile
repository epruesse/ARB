# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .depend
OBJECTS = aisc.o aisc_commands.o aisc_var_ref.o aisc_mix.o

BINARY=aisc

$(MAIN): $(BINARY)

$(BINARY):	$(OBJECTS)
	$(CPP) $(cflags) -o $@ $(OBJECTS)

.c.o:
	$(CPP) $(cflags) -c $< $(AINCLUDES)

DEPENDS = $(OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

clean:
	rm -f $(OBJECTS) $(BINARY)

proto:
	../AISC_MKPTPS/aisc_mkpt -C -g *.c >aisc_proto.h.tmp
	../SOURCE_TOOLS/mv_if_diff aisc_proto.h.tmp aisc_proto.h

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

aisc.o: aisc.h
aisc.o: aisc_proto.h

aisc_commands.o: aisc.h
aisc_commands.o: aisc_proto.h

aisc_mix.o: aisc.h
aisc_mix.o: aisc_proto.h

aisc_var_ref.o: aisc.h
aisc_var_ref.o: aisc_proto.h
