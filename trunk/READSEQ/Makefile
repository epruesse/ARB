OBJECT = readseq.o ureadseq.o

.SUFFIXES: .o .c .cxx

ifdef DEBIAN
all:
	@echo Do not compile readseq.  Instead use the Debian package.
else
all: $(ARBHOME)/bin/arb_readseq
endif

# --------------------------------------------------------------------------------

ifeq (1,1)
# no warnings in this directory
SUB_CPP=$(CPP:-W -Wall=-w)
SUB_ACC=$(ACC:-W -Wall=-w)
else
SUB_CPP=$(CPP)
SUB_ACC=$(ACC)
endif


$(ARBHOME)/bin/arb_readseq:  $(OBJECT)
	$(SUB_ACC) $(cflags) -o $@ $(OBJECT)

.c.o:
	$(SUB_ACC) $(cflags) -c $< $(AINCLUDES)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(OBJECT:.o=.c*)

# DO NOT DELETE THIS LINE -- make depend depends on it.

readseq.o:  ureadseq.h
ureadseq.o:  ureadseq.h
