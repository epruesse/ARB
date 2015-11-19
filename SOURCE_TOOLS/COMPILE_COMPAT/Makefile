# --------------------------------------------------------------------------------
# Compilations done here try to make sure, that ARB  stays compatible
# to compile environments used by other software that uses (parts of) ARB.
# --------------------------------------------------------------------------------
#
# for variables passed from parent makefile see ../parent_make.txt

.SUFFIXES: .o .cxx .depend
.PHONY: binaries clean depends depends.init

OBJECTS = c0x.o nostd.o

BINARIES = $(addprefix bin/,$(subst .o,,$(OBJECTS)))

LIB_ARBDB := $(LIBPATH) $(ARBDB_LIB) $(ARB_GLIB_LIBS)

LIBDIR=../../lib
LIB_DEPENDS := $(LIBDIR)/libARBDB.$(SHARED_LIB_SUFFIX) $(LIBDIR)/libCORE.$(SHARED_LIB_SUFFIX)

$(MAIN): binaries

binaries:
	mkdir -p bin
	$(MAKE) $(BINARIES)

cxxflags_c0x=$(subst -std=gnu++11,-std=gnu++0x,$(cxxflags))
cxxflags_nostd=$(subst -std=gnu++0x,,$(subst -std=gnu++11,,$(cxxflags)))

c0x.o: c0x.cxx Makefile
	$(A_CXX) $(cflags) $(cxxflags_c0x) -c $< $(CXX_INCLUDES) -o $@ $(POST_COMPILE)

nostd.o: nostd.cxx Makefile
	$(A_CXX) $(cflags) $(cxxflags_nostd) -c $< $(CXX_INCLUDES) -o $@ $(POST_COMPILE)

bin/% : %.o
	$(LINK_EXECUTABLE) $@ $< $(LIB_ARBDB)

$(OBJECTS) : $(LIB_DEPENDS)

clean:
	rm -f $(BINARIES) $(OBJECTS)

DEPENDS = $(OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

c0x.o: $(ARBHOME)/INCLUDE/ad_prot.h
c0x.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
c0x.o: $(ARBHOME)/INCLUDE/arb_assert.h
c0x.o: $(ARBHOME)/INCLUDE/arb_core.h
c0x.o: $(ARBHOME)/INCLUDE/arb_error.h
c0x.o: $(ARBHOME)/INCLUDE/arb_msg.h
c0x.o: $(ARBHOME)/INCLUDE/arb_string.h
c0x.o: $(ARBHOME)/INCLUDE/arbdb.h
c0x.o: $(ARBHOME)/INCLUDE/arbdb_base.h
c0x.o: $(ARBHOME)/INCLUDE/arbdbt.h
c0x.o: $(ARBHOME)/INCLUDE/arbtools.h
c0x.o: $(ARBHOME)/INCLUDE/attributes.h
c0x.o: $(ARBHOME)/INCLUDE/cxxforward.h
c0x.o: $(ARBHOME)/INCLUDE/downcast.h
c0x.o: $(ARBHOME)/INCLUDE/dupstr.h
c0x.o: $(ARBHOME)/INCLUDE/gccver.h
c0x.o: $(ARBHOME)/INCLUDE/smartptr.h
c0x.o: $(ARBHOME)/INCLUDE/static_assert.h
c0x.o: $(ARBHOME)/INCLUDE/test_global.h

nostd.o: $(ARBHOME)/INCLUDE/ad_prot.h
nostd.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
nostd.o: $(ARBHOME)/INCLUDE/arb_assert.h
nostd.o: $(ARBHOME)/INCLUDE/arb_core.h
nostd.o: $(ARBHOME)/INCLUDE/arb_error.h
nostd.o: $(ARBHOME)/INCLUDE/arb_msg.h
nostd.o: $(ARBHOME)/INCLUDE/arb_string.h
nostd.o: $(ARBHOME)/INCLUDE/arbdb.h
nostd.o: $(ARBHOME)/INCLUDE/arbdb_base.h
nostd.o: $(ARBHOME)/INCLUDE/arbdbt.h
nostd.o: $(ARBHOME)/INCLUDE/arbtools.h
nostd.o: $(ARBHOME)/INCLUDE/attributes.h
nostd.o: $(ARBHOME)/INCLUDE/cxxforward.h
nostd.o: $(ARBHOME)/INCLUDE/downcast.h
nostd.o: $(ARBHOME)/INCLUDE/dupstr.h
nostd.o: $(ARBHOME)/INCLUDE/gccver.h
nostd.o: $(ARBHOME)/INCLUDE/smartptr.h
nostd.o: $(ARBHOME)/INCLUDE/static_assert.h
nostd.o: $(ARBHOME)/INCLUDE/test_global.h
