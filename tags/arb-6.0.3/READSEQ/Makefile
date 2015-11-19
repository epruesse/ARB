.SUFFIXES: .o .c .cxx .depend

OBJECT = readseq.o ureadseq.o

BINARY=$(ARBHOME)/bin/arb_readseq

ifeq ($(DEBIAN),1)
all:
	@echo Do not compile readseq.  Instead use the Debian package.
else
all: $(BINARY)
endif

# --------------------------------------------------------------------------------

# no warnings in this directory
subcflags:=$(subst -W -Wall,-w,$(cflags))


$(BINARY):  $(OBJECT)
	$(A_CC) $(subcflags) -o $@ $(OBJECT)

.c.o:
	$(A_CC) $(subcflags) -c $< $(CC_INCLUDES) $(POST_COMPILE)

clean:
	rm -f $(OBJECT) $(BINARY) 

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

readseq.o: ureadseq.h

ureadseq.o: ureadseq.h
