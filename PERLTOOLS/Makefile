.SUFFIXES: .c .cxx .depend .o

TOOLS := \
	arb_proto_2_xsub \

LIB_ARBDB            := $(LIBPATH) $(ARBDB_LIB) $(ARB_GLIB_LIBS)
LIBS_ARB_PROBE       := ../SERVERCNTRL/SERVERCNTRL.a ../PROBE_COM/client.a
LIBS_ARB_READ_TREE   := ../SL/TREE_READ/TREE_READ.a
LIBS_ARB_EXPORT_TREE := ../SL/TREE_WRITE/TREE_WRITE.a ../XML/XML.a $(LIBS_ARB_READ_TREE)

OBJECTS:=$(addsuffix .o,$(TOOLS))
BINARIES:=$(addprefix $(ARBHOME)/bin/,$(TOOLS))

all: $(BINARIES)

%.o: %.cxx 
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE)

%.o: %.c
	$(A_CC) $(cflags) -c $< $(CC_INCLUDES) $(POST_COMPILE)

$(ARBHOME)/bin/%: %.o $(use_ARB_main)
	$(LINK_EXECUTABLE) $@ $(use_ARB_main) $< $(LIB_ARBDB) $(EXECLIBS)

clean:
	rm -f $(BINARIES)
	rm -f $(OBJECTS)

proto:
	@echo "Nothing todo for proto in PERLTOOLS"

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/arb_diff.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/arb_str.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/arb_string.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/attributes.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/gccver.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/test_global.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/test_unit.h
arb_proto_2_xsub.o: $(ARBHOME)/INCLUDE/ut_valgrinded.h
