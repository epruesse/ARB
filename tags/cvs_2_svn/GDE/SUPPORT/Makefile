.SUFFIXES: .o .c .depend

OBJECT =  CAP2.o Restriction.o Zuk_to_gen.o count.o findall.o lsadt.o \
	sho_helix.o varpos.o

BINARIES=$(OBJECT:%.o=$(ARBHOME)/bin/%)

all: $(BINARIES)

$(ARBHOME)/bin/%: %.c
	$(ACC) $(cflags) -o $@ $< $(AINCLUDES) $(LIB) -lm

clean:
	rm -f $(BINARIES)

DEPENDS = $(OBJECT:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@


# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

count.o: Flatio.c

findall.o: Flatio.c

Restriction.o: Flatio.c

sho_helix.o: Flatio.c

varpos.o: Flatio.c
