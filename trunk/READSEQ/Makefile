OBJECT = readseq.o ureadseq.o

.SUFFIXES: .o .c .cxx

ifndef DEBIAN
all: $(ARBHOME)/bin/arb_readseq
else
all:
	@echo Do not compile readseq.  Instead use the Debian package.
endif

$(ARBHOME)/bin/arb_readseq:  $(OBJECT)
	$(ACC) $(cflags) -o $@ $(OBJECT)

.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(OBJECT:.o=.c*)

# DO NOT DELETE THIS LINE -- make depend depends on it.

readseq.o:  ureadseq.h
ureadseq.o:  ureadseq.h
