.SUFFIXES: .c .cxx .depend .o

TOOLS := \
	slv_arb_export \
	slv_arb_probe \

LIB_ARBDB            := $(LIBPATH) $(ARBDB_LIB)
LIBS_ARB_PROBE       := ../PROBE_COM/client.a
LIBS_ARB_EXPORT      := ../SL/FILTER/FILTER.a ../SL/SEQIO/SEQIO.a ../XML/XML.a

OBJECTS:=$(addsuffix .o,$(TOOLS))
BINARIES:=$(addprefix $(ARBHOME)/bin/,$(TOOLS))

all: $(BINARIES)

%.o: %.cxx
	$(CPP) $(cflags) -c $< $(CPPINCLUDES) $(POST_COMPILE)

%.o: %.c
	$(ACC) $(cflags) -c $< $(CPPINCLUDES) $(POST_COMPILE)

$(ARBHOME)/bin/slv_arb_probe: slv_arb_probe.o $(LIBS_ARB_PROBE)
	$(LINK_EXECUTABLE) $@ $< $(LIBS_ARB_PROBE) $(LIB_ARBDB) $(EXECLIBS)
$(ARBHOME)/bin/slv_arb_export: slv_arb_export.o $(LIBS_ARB_PROBE)
	$(LINK_EXECUTABLE) $@ $< $(LIBS_ARB_EXPORT) $(LIB_ARBDB) $(EXECLIBS)

$(ARBHOME)/bin/%: %.o
	$(LINK_EXECUTABLE) $@ $< $(LIB_ARBDB) $(EXECLIBS)

clean:
	rm -f $(BINARIES)
	rm -f $(OBJECTS)

depends:
	$(MAKE) "DEBUG=1" depends_debug

DEPENDS = $(OBJECTS:.o=.depend)
depends_debug: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

