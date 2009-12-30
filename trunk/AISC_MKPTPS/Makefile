# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

OBJECTS = mkptypes.o
BINARY = aisc_mkpt

$(MAIN): $(BINARY)

$(BINARY): $(OBJECTS)
	$(LINK_EXECUTABLE) $@ $(OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(POST_COMPILE)

clean:
	rm -f $(OBJECTS) $(BINARY)

depends:
	@echo "No depends here"

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl
