# Makefile May 2004 by Alexandros P. Stamatakis
# modified for ARB 2005

.SUFFIXES: .o .c .h .depend

RAXML_OBJECTS=axml.o dnapars.o rev_functions.o
OBJECTS = $(RAXML_OBJECTS)
RM 	   = rm -f

CFLAGS_RAXML = -DNORMAL -fomit-frame-pointer -funroll-loops

# ----------------------------------------
# normal compilation:
ifndef ARB

CC 		= gcc
CFLAGS 	= -O3 $(CFLAGS_RAXML)
TARGET  = .
MAIN 	= $(EXE)
#-xO5
# -pg
LDFLAGS = -lm

# ----------------------------------------
# compile for ARB:
else

CC      = $(ACC)
TARGET 	= $(ARBHOME)/bin
LDFLAGS = $(LIB) -lm
CFLAGS  = $(cflags) $(CFLAGS_RAXML)

$(MAIN) : all

endif

TARGETS = $(TARGET)/raxml # $(TARGET)/consense #$(TARGET)/raxmlOMP $(TARGET)/raxmlMKL

all : $(TARGETS)

$(TARGET)/consense : consense.o cons.o phylip.o
	$(CC) $(CFLAGS)	-o $(TARGET)/consense $(LDFLAGS) consense.o phylip.o cons.o

$(TARGET)/raxml :  $(RAXML_OBJECTS)
	$(CC) $(CFLAGS) -o  $(TARGET)/raxml $(RAXML_OBJECTS) $(LDFLAGS)

$(TARGET)/raxmlOMP :axmlOMP.o dnaparsOMP.o rev_functions.o
	$(CC) $(CFLAGS_OMP) -o $(TARGET)/raxmlOMP axmlOMP.o dnaparsOMP.o rev_functions.o $(LDFLAGS)

$(TARGET)/raxmlMKL : axmlMKL.o dnapars.o rev_functions.o
	$(CC) $(CFLAGS_MKL) -o  $(TARGET)/raxmlMKL axmlMKL.o dnapars.o rev_functions.o $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean :
	$(RM) $(OBJECTS) $(TARGETS)

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

axml.o: axml.h

cons.o: cons.h
cons.o: phylip.h

consense.o: cons.h
consense.o: phylip.h

dnapars.o: phylip2.h

phylip.o: phylip.h
