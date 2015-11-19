# The file, where all structures are described
MAIN_SOURCE = PT.aisc
MAIN_HEADER = aisc_include.header

# Your own server functions
PRIVATE_SERVER_OBJECTS := PT_extern.o

# Public server/client sources (needed for dependencies)
PUBLIC_SOURCES := $(wildcard C/*.c) $(wildcard C/*.h)

# Do you want to create save and load
AISC_SAVE := NO

# C++ Compiler
COMPILER := $(A_CXX) -x c++ $(cflags) $(cxxflags) $(CXX_INCLUDES)
CLIENTCOMPILER := $(COMPILER)

# AISC dependencies
AISC_COMPILER=../AISC/aisc
AISC_PROTOTYPER=../AISC_MKPTPS/aisc_mkpt
AISC_DEPENDS = $(wildcard AISC/*.pa) $(AISC_COMPILER) $(AISC_PROTOTYPER)

include AISC/export2sub

$(MAIN): server.a

server.a: $(MAIN_SOURCE) $(PRIVATE_SERVER_OBJECTS:.o=.c) $(PUBLIC_SOURCES) $(AISC_DEPENDS) $(MAIN_HEADER)
	@$(MAKE) -r -f AISC/Makefile "AUTODEPENDS=0"

depends:
	@rm -f .depends
	@$(MAKE) -r -f AISC/Makefile "AUTODEPENDS=2" pregenerate

clean:
	@rm -f .depends
	@$(MAKE) -r -f AISC/Makefile clean

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

aisc_extern.o: aisc_extern_privat.h
aisc_extern.o: aisc_func_types.h
aisc_extern.o: aisc_global.h
aisc_extern.o: GENH/aisc.h
aisc_extern.o: server.h
aisc_extern.o: $(ARBHOME)/INCLUDE/arb_assert.h
aisc_extern.o: $(ARBHOME)/INCLUDE/arb_core.h
aisc_extern.o: $(ARBHOME)/INCLUDE/arbtools.h
aisc_extern.o: $(ARBHOME)/INCLUDE/attributes.h
aisc_extern.o: $(ARBHOME)/INCLUDE/bytestring.h
aisc_extern.o: $(ARBHOME)/INCLUDE/cxxforward.h
aisc_extern.o: $(ARBHOME)/INCLUDE/dupstr.h
aisc_extern.o: $(ARBHOME)/INCLUDE/gccver.h
aisc_extern.o: $(ARBHOME)/INCLUDE/test_global.h

client.o: aisc_global.h
client.o: client.h
client.o: client_privat.h
client.o: client_types.h
client.o: common.h
client.o: trace.h
client.o: $(ARBHOME)/INCLUDE/arb_assert.h
client.o: $(ARBHOME)/INCLUDE/arb_core.h
client.o: $(ARBHOME)/INCLUDE/arb_cs.h
client.o: $(ARBHOME)/INCLUDE/arbtools.h
client.o: $(ARBHOME)/INCLUDE/attributes.h
client.o: $(ARBHOME)/INCLUDE/bytestring.h
client.o: $(ARBHOME)/INCLUDE/cxxforward.h
client.o: $(ARBHOME)/INCLUDE/dupstr.h
client.o: $(ARBHOME)/INCLUDE/gccver.h
client.o: $(ARBHOME)/INCLUDE/SigHandler.h
client.o: $(ARBHOME)/INCLUDE/test_global.h

common.o: common.h
common.o: $(ARBHOME)/INCLUDE/arb_assert.h
common.o: $(ARBHOME)/INCLUDE/arb_core.h
common.o: $(ARBHOME)/INCLUDE/arb_cs.h
common.o: $(ARBHOME)/INCLUDE/arb_msg.h
common.o: $(ARBHOME)/INCLUDE/attributes.h
common.o: $(ARBHOME)/INCLUDE/dupstr.h
common.o: $(ARBHOME)/INCLUDE/test_global.h

GENC/aisc_global.o: aisc_func_types.h
GENC/aisc_global.o: aisc_global.h
GENC/aisc_global.o: aisc_server.h
GENC/aisc_global.o: GENH/aisc.h
GENC/aisc_global.o: GENH/aisc_com.h
GENC/aisc_global.o: GENH/aisc_server_extern.h
GENC/aisc_global.o: GENH/aisc_server_proto.h
GENC/aisc_global.o: GENH/import_proto.h
GENC/aisc_global.o: server.h
GENC/aisc_global.o: struct_man.h
GENC/aisc_global.o: $(ARBHOME)/INCLUDE/arb_assert.h
GENC/aisc_global.o: $(ARBHOME)/INCLUDE/arb_core.h
GENC/aisc_global.o: $(ARBHOME)/INCLUDE/arbtools.h
GENC/aisc_global.o: $(ARBHOME)/INCLUDE/attributes.h
GENC/aisc_global.o: $(ARBHOME)/INCLUDE/bytestring.h
GENC/aisc_global.o: $(ARBHOME)/INCLUDE/cxxforward.h
GENC/aisc_global.o: $(ARBHOME)/INCLUDE/dupstr.h
GENC/aisc_global.o: $(ARBHOME)/INCLUDE/gccver.h
GENC/aisc_global.o: $(ARBHOME)/INCLUDE/test_global.h

GENC/aisc_server.o: aisc_func_types.h
GENC/aisc_server.o: aisc_global.h
GENC/aisc_server.o: GENH/aisc.h
GENC/aisc_server.o: GENH/aisc_com.h
GENC/aisc_server.o: GENH/aisc_server_extern.h
GENC/aisc_server.o: GENH/aisc_server_proto.h
GENC/aisc_server.o: GENH/import_proto.h
GENC/aisc_server.o: server.h
GENC/aisc_server.o: struct_man.h
GENC/aisc_server.o: $(ARBHOME)/INCLUDE/arb_assert.h
GENC/aisc_server.o: $(ARBHOME)/INCLUDE/arb_core.h
GENC/aisc_server.o: $(ARBHOME)/INCLUDE/arbtools.h
GENC/aisc_server.o: $(ARBHOME)/INCLUDE/attributes.h
GENC/aisc_server.o: $(ARBHOME)/INCLUDE/bytestring.h
GENC/aisc_server.o: $(ARBHOME)/INCLUDE/cxxforward.h
GENC/aisc_server.o: $(ARBHOME)/INCLUDE/dupstr.h
GENC/aisc_server.o: $(ARBHOME)/INCLUDE/gccver.h
GENC/aisc_server.o: $(ARBHOME)/INCLUDE/test_global.h

server.o: aisc_func_types.h
server.o: aisc_global.h
server.o: common.h
server.o: GENH/aisc_com.h
server.o: server.h
server.o: trace.h
server.o: $(ARBHOME)/INCLUDE/arb_assert.h
server.o: $(ARBHOME)/INCLUDE/arb_core.h
server.o: $(ARBHOME)/INCLUDE/arb_cs.h
server.o: $(ARBHOME)/INCLUDE/arbtools.h
server.o: $(ARBHOME)/INCLUDE/attributes.h
server.o: $(ARBHOME)/INCLUDE/bytestring.h
server.o: $(ARBHOME)/INCLUDE/cxxforward.h
server.o: $(ARBHOME)/INCLUDE/dupstr.h
server.o: $(ARBHOME)/INCLUDE/gccver.h
server.o: $(ARBHOME)/INCLUDE/SigHandler.h
server.o: $(ARBHOME)/INCLUDE/static_assert.h
server.o: $(ARBHOME)/INCLUDE/test_global.h

struct_man.o: aisc_global.h
struct_man.o: GENH/aisc.h
struct_man.o: struct_man.h
struct_man.o: $(ARBHOME)/INCLUDE/arb_assert.h
struct_man.o: $(ARBHOME)/INCLUDE/arb_core.h
struct_man.o: $(ARBHOME)/INCLUDE/arbtools.h
struct_man.o: $(ARBHOME)/INCLUDE/attributes.h
struct_man.o: $(ARBHOME)/INCLUDE/bytestring.h
struct_man.o: $(ARBHOME)/INCLUDE/cxxforward.h
struct_man.o: $(ARBHOME)/INCLUDE/dupstr.h
struct_man.o: $(ARBHOME)/INCLUDE/gccver.h
struct_man.o: $(ARBHOME)/INCLUDE/test_global.h
