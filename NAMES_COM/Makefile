# The file, where all structures are described
MAIN_SOURCE = names.aisc

# Your own server functions
PRIVATE_SERVER_OBJECTS = names_extern.o

# Public server/client sources (needed for dependencies)
PUBLIC_SOURCES = $(wildcard C/*.c)

# Do you want to create save and load
AISC_SAVE	= YES
COMPILER = $(CPP) $(cflags)
CLIENTCOMPILER = $(CPP) $(cflags)

# DONT CHANGE THE NEXT LINES
server.a: $(MAIN_SOURCE) $(PRIVATE_SERVER_OBJECTS:.o=.c) $(PUBLIC_SOURCES)
	$(MAKE) -r $(makeflags) -f AISC/Makefile \
				"MAIN_SOURCE=$(MAIN_SOURCE)" \
				"PRIVATE_SERVER_OBJECTS = $(PRIVATE_SERVER_OBJECTS)" \
				"AISC_SAVE = $(AISC_SAVE)" "COMPILER = $(COMPILER)" \
				"CLIENTCOMPILER = $(CLIENTCOMPILER)" "AR = $(AR)"

depends:
	$(MAKE) -r -f AISC/Makefile depends

clean:
	$(MAKE) $(makeflags) -f AISC/Makefile clean

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl
