OBJECT = readseq.o ureadseq.o

.SUFFIXES: .o .c .cxx .depend

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

DEPENDS = $(OBJECT:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

readseq.o: ureadseq.h

ureadseq.o: ureadseq.h
