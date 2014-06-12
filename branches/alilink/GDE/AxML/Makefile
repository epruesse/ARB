# Makefile February 2002 by Alexandros P. Stamatakis

.SUFFIXES: .o .c .h .depend

OBJECTS = axml.o
RM 	   = rm -f

# ----------------------------------------
# normal compilation:
ifndef ARB

CC 		= gcc
CFLAGS 	= -O3
TARGET  = axml
MAIN 	= $(EXE)

# ----------------------------------------
# compile for ARB:
else

CC      = $(A_CC)
TARGET 	= $(ARBHOME)/bin/axml
CFLAGS  = $(cflags)

$(MAIN) : all

endif

# ----------------------------------------

all : $(TARGET)

$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< -lm

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean :
	$(RM) $(OBJECTS) $(TARGET)

DEPENDS = $(OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from GDE)

axml.o: axml.h
