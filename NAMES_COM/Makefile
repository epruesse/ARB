# The file, where all structures are described
MAIN_SOURCE = names.aisc

# Your own server functions
PRIVATE_SERVER_OBJECTS = names_extern.o

# Do you want to create save and load
AISC_SAVE	= YES
COMPILER = $(CPP) $(cflags)
CLIENTCOMPILER = $(CPP) $(cflags)

# DONT CHANGE THE NEXT LINES
server.a:	$(MAIN_SOURCE)	$(PRIVATE_SERVER_OBJECTS:.o=.c)
		$(MAKE) -r $(makeflags) -f AISC/Makefile \
				"MAIN_SOURCE=$(MAIN_SOURCE)" \
				"PRIVATE_SERVER_OBJECTS = $(PRIVATE_SERVER_OBJECTS)" \
				"AISC_SAVE = $(AISC_SAVE)" "COMPILER = $(COMPILER)" \
				"CLIENTCOMPILER = $(CLIENTCOMPILER)" "AR = $(AR)"

depends:
	@echo "no automatic dependencies for NAMES_COM - skipped"

clean:
	$(MAKE) $(makeflags) -f AISC/Makefile clean

# DO NOT DELETE
