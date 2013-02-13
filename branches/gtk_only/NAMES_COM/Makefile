# The file, where all structures are described
MAIN_SOURCE = names.aisc
MAIN_HEADER = aisc_include.header

# Your own server functions
PRIVATE_SERVER_OBJECTS = names_extern.o

# Public server/client sources (needed for dependencies)
PUBLIC_SOURCES := $(wildcard C/*.c) $(wildcard C/*.h)

# Do you want to create save and load
AISC_SAVE := YES

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
	@$(MAKE) -r -f AISC/Makefile "AUTODEPENDS=2" pregenerate

clean:
	@rm -f .depends
	@$(MAKE) -r -f AISC/Makefile clean

# DO NOT DELETE

C/aisc_extern.o: C/server.h /home/epruesse/arb/gtk_merge/INCLUDE/attributes.h
C/aisc_extern.o: /home/epruesse/arb/gtk_merge/INCLUDE/gccver.h
C/aisc_extern.o: C/aisc_func_types.h C/aisc_extern_privat.h
C/client.o: /home/epruesse/arb/gtk_merge/INCLUDE/arb_cs.h
C/client.o: /home/epruesse/arb/gtk_merge/INCLUDE/arb_core.h
C/client.o: /home/epruesse/arb/gtk_merge/INCLUDE/dupstr.h C/client_privat.h
C/client.o: C/aisc_global.h /home/epruesse/arb/gtk_merge/INCLUDE/bytestring.h
C/client.o: /home/epruesse/arb/gtk_merge/INCLUDE/arbtools.h
C/client.o: /home/epruesse/arb/gtk_merge/INCLUDE/cxxforward.h
C/client.o: /home/epruesse/arb/gtk_merge/INCLUDE/gccver.h
C/client.o: /home/epruesse/arb/gtk_merge/INCLUDE/attributes.h
C/client.o: /home/epruesse/arb/gtk_merge/INCLUDE/arb_assert.h
C/client.o: /home/epruesse/arb/gtk_merge/INCLUDE/test_global.h
C/client.o: /home/epruesse/arb/gtk_merge/INCLUDE/SigHandler.h C/client.h
C/client.o: C/client_types.h C/trace.h
C/server.o: C/trace.h C/server.h
C/server.o: /home/epruesse/arb/gtk_merge/INCLUDE/attributes.h
C/server.o: /home/epruesse/arb/gtk_merge/INCLUDE/gccver.h C/aisc_func_types.h
C/server.o: /home/epruesse/arb/gtk_merge/INCLUDE/SigHandler.h
C/server.o: /home/epruesse/arb/gtk_merge/INCLUDE/arb_cs.h
C/server.o: /home/epruesse/arb/gtk_merge/INCLUDE/arb_core.h
C/server.o: /home/epruesse/arb/gtk_merge/INCLUDE/dupstr.h
C/server.o: /home/epruesse/arb/gtk_merge/INCLUDE/static_assert.h
C/server.o: /home/epruesse/arb/gtk_merge/INCLUDE/cxxforward.h
C/struct_man.o: C/struct_man.h
C/struct_man.o: /home/epruesse/arb/gtk_merge/INCLUDE/attributes.h
C/struct_man.o: /home/epruesse/arb/gtk_merge/INCLUDE/gccver.h
