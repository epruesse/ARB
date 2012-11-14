# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .depend

OBJECTS = mkptypes.o
BINARY = aisc_mkpt

$(MAIN): $(BINARY)

$(BINARY): $(OBJECTS)
	$(ACC) $(cflags) -o $@ $(OBJECTS)

.c.o:
	$(ACC) $(cflags) -c $< $(POST_COMPILE)

clean:
	rm -f $(OBJECTS) $(BINARY)

depends:
	@echo "No depends here"

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl
