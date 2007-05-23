

# The file, where all structures are described
MAIN_SOURCE = mgr.aisc

# Your own server functions
PRIVATE_SERVER_OBJECTS = mgr_extern.o

# Public server/client sources (needed for dependencies)
PUBLIC_SOURCES = $(wildcard C/*.c)

# Do you want to create save and load
AISC_SAVE	= NO

server.a:	$(MAIN_SOURCE)	$(PRIVATE_SERVER_OBJECTS:.o=.c) $(PUBLIC_SOURCES)
	make $(makeflags) -f AISC/Makefile "MAIN_SOURCE=$(MAIN_SOURCE)" "PRIVATE_SERVER_OBJECTS = $(PRIVATE_SERVER_OBJECTS)" "AISC_SAVE = $(AISC_SAVE)"

proto:
		$(MAKE) -C C proto

depends:
		touch .depends_new # dummy .depends
		$(ARBHOME)/SOURCE_TOOLS/mv_if_diff .depends_new .depends

clean:
	make $(makeflags) -f AISC/Makefile clean
