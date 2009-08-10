# The file, where all structures are described
MAIN_SOURCE := PT.aisc

# Your own server functions
PRIVATE_SERVER_OBJECTS := PT_extern.o

# Public server/client sources (needed for dependencies)
PUBLIC_SOURCES := $(wildcard C/*.c)

# Do you want to create save and load
AISC_SAVE := NO

# C++ Compiler
COMPILER := $(CPP) $(cflags) -I$(ARBHOME)/INCLUDE
CLIENTCOMPILER := $(COMPILER)

# AISC dependencies
AISC_COMPILER=../MAKEBIN/aisc
AISC_DEPENDS = $(wildcard AISC/*.pa) $(AISC_COMPILER)

include AISC/export2sub

server.a:	$(MAIN_SOURCE)	$(PRIVATE_SERVER_OBJECTS:.o=.c) $(PUBLIC_SOURCES) $(AISC_DEPENDS)
		@$(MAKE) -r -f AISC/Makefile

depends:
		@$(MAKE) -r -f AISC/Makefile pregenerate

clean:
		@$(MAKE) -r -f AISC/Makefile clean

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl
