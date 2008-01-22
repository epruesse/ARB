# Makefile August 2006 by Alexandros Stamatakis
# Modified for compilation in ARB, Harald Meier 2008-01-21

.SUFFIXES: .o .c .h .depend

RAXML_OBJECTS    = axml.o raxmlParsimony.o rev_functions.o optimizeModel.o multiple.o searchAlgo.o topologies.o parsePartitions.o treeIO.o models.o bipartitionList.o rapidBootstrap.o evaluatePartialGeneric.o evaluateGeneric.o newviewGeneric.o makenewzGeneric.o  evaluateGenericVector.o categorizeGeneric.o 
OBJECTS = $(RAXML_OBJECTS)
RM = rm -f

GLOBAL_DEPS = axml.h globalVariables.h
CFLAGS_RAXML =  -fomit-frame-pointer -funroll-loops  
#-Wall -pedantic -Wunused-parameter -Wredundant-decls  -Wreturn-type  -Wswitch-default -Wunused-value -Wimplicit  -Wimplicit-function-declaration  -Wimplicit-int -Wimport -pedantic-errors -Wunused  -Wunused-function  -Wunused-label -Wno-int-to-pointer-cast -Wbad-function-cast  -Wmissing-declarations -Wmissing-prototypes  -Wnested-externs  -Wold-style-definition -Wstrict-prototypes  -Wdeclaration-after-statement -Wpointer-sign -Wextra -Wmissing-noreturn -Wredundant-decls -Wunused -Wunused-function -Wunused-parameter -Wunused-value  -Wunused-variable

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

TARGETS = $(TARGET)/raxmlHPC
all : $(TARGETS)

$(TARGET)/raxmlHPC :  $(RAXML_OBJECTS) $(GLOBAL_DEPS)
	$(CC) $(CFLAGS) -o  $(TARGET)/raxmlHPC $(RAXML_OBJECTS) $(LDFLAGS)
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



