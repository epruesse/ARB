# The file, where all structures are described
MAIN_SOURCE = PT.aisc

# Your own server functions
PRIVATE_SERVER_OBJECTS = PT_extern.o

# Do you want to create save and load
AISC_SAVE	= NO

COMPILER = $(CPP) $(cflags)
CLIENTCOMPILER = $(CPP) $(cflags)

# DONT CHANGE THE NEXT LINES

server.a:	$(MAIN_SOURCE)	$(PRIVATE_SERVER_OBJECTS:.o=.c)
		$(MAKE) -r $(makeflags) -f AISC/Makefile \
				"MAIN_SOURCE=$(MAIN_SOURCE)" \
				"PRIVATE_SERVER_OBJECTS = $(PRIVATE_SERVER_OBJECTS)" \
				"AISC_SAVE = $(AISC_SAVE)" "COMPILER = $(COMPILER)" \
				"CLIENTCOMPILER = $(CLIENTCOMPILER)" "AR = $(AR)"

depend:
	@echo "MGR COM no automatic dependencies"

clean:
	$(MAKE) $(makeflags) -f AISC/Makefile clean
