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

io_arbdb.o: $(ARBHOME)/INCLUDE/arbdb.h
io_arbdb.o: $(ARBHOME)/INCLUDE/arb_assert.h
io_arbdb.o: $(ARBHOME)/INCLUDE/ad_prot.h
io_arbdb.o: $(ARBHOME)/INCLUDE/arbdbt.h
io_arbdb.o: $(ARBHOME)/INCLUDE/ad_t_prot.h ring.h getopt.h xmalloc.h
io_arbdb.o: lenstring.h hash.h careful.h

getopt.o: getopt.h

getopt1.o: getopt.h

xmalloc.o: xmalloc.h

lenstring.o: lenstring.h xmalloc.h
