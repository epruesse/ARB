

# The file, where all structures are described
MAIN_SOURCE = mgr.aisc

# Your own server functions
PRIVATE_SERVER_OBJECTS = mgr_extern.o

# Do you want to create save and load
AISC_SAVE	= NO

server.a:	$(MAIN_SOURCE)	$(PRIVATE_SERVER_OBJECTS:.o=.c)
	make $(makeflags) -f AISC/Makefile "MAIN_SOURCE=$(MAIN_SOURCE)" "PRIVATE_SERVER_OBJECTS = $(PRIVATE_SERVER_OBJECTS)" "AISC_SAVE = $(AISC_SAVE)"

clean:
	make $(makeflags) -f AISC/Makefile clean
