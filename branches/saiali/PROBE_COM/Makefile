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
COMPILER := $(CPP) -x c++ $(cflags) -I$(ARBHOME)/INCLUDE
CLIENTCOMPILER := $(COMPILER)

# AISC dependencies
AISC_COMPILER=../AISC/aisc
AISC_PROTOTYPER=../AISC_MKPTPS/aisc_mkpt
AISC_DEPENDS = $(wildcard AISC/*.pa) $(AISC_COMPILER) $(AISC_PROTOTYPER)

include AISC/export2sub

$(MAIN): server.a

server.a: $(MAIN_SOURCE) $(PRIVATE_SERVER_OBJECTS:.o=.c) $(PUBLIC_SOURCES) $(AISC_DEPENDS) $(MAIN_HEADER)
	@$(MAKE) -r -f AISC/Makefile

depends:
	@rm -f .depends
	@$(MAKE) -r -f AISC/Makefile pregenerate

clean:
	@rm -f .depends
	@$(MAKE) -r -f AISC/Makefile clean

# DO NOT DELETE

C/aisc_extern.o: GENH/aisc.h C/aisc_global.h
C/aisc_extern.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/bytestring.h
C/aisc_extern.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arbtools.h
C/aisc_extern.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/attributes.h
C/aisc_extern.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arb_assert.h
C/aisc_extern.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arb_core.h
C/aisc_extern.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/dupstr.h
C/aisc_extern.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/test_global.h
C/aisc_extern.o: C/server.h C/aisc_func_types.h C/aisc_extern_privat.h
C/client.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arb_cs.h
C/client.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arb_core.h
C/client.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/dupstr.h
C/client.o: C/client_privat.h C/aisc_global.h
C/client.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/bytestring.h
C/client.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arbtools.h
C/client.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/attributes.h
C/client.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arb_assert.h
C/client.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/test_global.h
C/client.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/SigHandler.h
C/client.o: C/client.h C/client_types.h C/trace.h
C/server.o: C/trace.h GENH/aisc_com.h C/aisc_global.h
C/server.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/bytestring.h
C/server.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arbtools.h
C/server.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/attributes.h
C/server.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arb_assert.h
C/server.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arb_core.h
C/server.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/dupstr.h
C/server.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/test_global.h
C/server.o: C/server.h C/aisc_func_types.h
C/server.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/SigHandler.h
C/server.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arb_cs.h
C/server.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/static_assert.h
C/struct_man.o: GENH/aisc.h C/aisc_global.h
C/struct_man.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/bytestring.h
C/struct_man.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arbtools.h
C/struct_man.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/attributes.h
C/struct_man.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arb_assert.h
C/struct_man.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arb_core.h
C/struct_man.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/dupstr.h
C/struct_man.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/test_global.h
C/struct_man.o: C/struct_man.h
GENC/aisc_global.o: C/aisc_server.h
GENC/aisc_global.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/dupstr.h
GENC/aisc_global.o: GENH/aisc.h C/aisc_global.h
GENC/aisc_global.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/bytestring.h
GENC/aisc_global.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arbtools.h
GENC/aisc_global.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/attributes.h
GENC/aisc_global.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arb_assert.h
GENC/aisc_global.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arb_core.h
GENC/aisc_global.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/test_global.h
GENC/aisc_global.o: GENH/aisc_com.h C/server.h C/aisc_func_types.h
GENC/aisc_global.o: GENH/aisc_server_proto.h GENH/aisc_server_extern.h
GENC/aisc_global.o: GENH/import_proto.h C/struct_man.h
GENC/aisc_server.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/dupstr.h
GENC/aisc_server.o: GENH/aisc.h C/aisc_global.h
GENC/aisc_server.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/bytestring.h
GENC/aisc_server.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arbtools.h
GENC/aisc_server.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/attributes.h
GENC/aisc_server.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arb_assert.h
GENC/aisc_server.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/arb_core.h
GENC/aisc_server.o: /home/ralf/ARB-bilbo/ARB.saiali.merge/INCLUDE/test_global.h
GENC/aisc_server.o: GENH/aisc_com.h C/server.h C/aisc_func_types.h
GENC/aisc_server.o: GENH/aisc_server_proto.h GENH/aisc_server_extern.h
GENC/aisc_server.o: GENH/import_proto.h C/struct_man.h
