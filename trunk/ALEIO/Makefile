LIB = -L../LIBLINK -lARBDO $(SYSLIBS)

OBJECT = io_arbdb.o getopt.o getopt1.o xmalloc.o lenstring.o careful.o

.SUFFIXES: .o .c .depend

all: $(ARBHOME)/bin/arb_aleio

$(ARBHOME)/bin/arb_aleio:  $(OBJECT)
	$(ACC) $(cflags) -o $@ $(OBJECT)  $(LIB)

.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES)

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

getopt.o: getopt.h

getopt1.o: getopt.h

io_arbdb.o: careful.h
io_arbdb.o: getopt.h
io_arbdb.o: hash.h
io_arbdb.o: lenstring.h
io_arbdb.o: ring.h
io_arbdb.o: xmalloc.h
io_arbdb.o: $(ARBHOME)/INCLUDE/ad_prot.h
io_arbdb.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
io_arbdb.o: $(ARBHOME)/INCLUDE/arb_assert.h
io_arbdb.o: $(ARBHOME)/INCLUDE/arbdb.h
io_arbdb.o: $(ARBHOME)/INCLUDE/arbdbt.h

lenstring.o: lenstring.h
lenstring.o: xmalloc.h

xmalloc.o: xmalloc.h
