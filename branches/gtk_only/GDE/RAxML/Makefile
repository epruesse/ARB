# Makefile August 2006 by Alexandros Stamatakis
# Modified for compilation in ARB, Harald Meier 2008-01-21

.SUFFIXES: .o .c .h .depend

RAXML_OBJECTS    = axml.o  optimizeModel.o multiple.o searchAlgo.o topologies.o parsePartitions.o treeIO.o models.o bipartitionList.o rapidBootstrap.o evaluatePartialGenericSpecial.o evaluateGenericSpecial.o newviewGenericSpecial.o makenewzGenericSpecial.o   classify.o fastDNAparsimony.o fastSearch.o leaveDropping.o rogueEPA.o ancestralStates.o mem_alloc.o
OBJECTS = $(RAXML_OBJECTS)
RM = rm -f

GLOBAL_DEPS = axml.h globalVariables.h #mem_alloc.h
CFLAGS_RAXML =  -fomit-frame-pointer -funroll-loops -O2 -msse #-Wall -Wunused-parameter -Wredundant-decls  -Wreturn-type  -Wswitch-default -Wunused-value -Wimplicit  -Wimplicit-function-declaration  -Wimplicit-int -Wimport  -Wunused  -Wunused-function  -Wunused-label -Wno-int-to-pointer-cast -Wbad-function-cast  -Wmissing-declarations -Wmissing-prototypes  -Wnested-externs  -Wold-style-definition -Wstrict-prototypes -Wpointer-sign -Wextra -Wredundant-decls -Wunused -Wunused-function -Wunused-parameter -Wunused-value  -Wunused-variable -Wformat  -Wformat-nonliteral -Wparentheses -Wsequence-point -Wuninitialized -Wundef -Wbad-function-cast

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

CC      = $(A_CC)
TARGET 	= $(ARBHOME)/bin
LDFLAGS = -lm
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



# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from GDE)

ancestralStates.o: axml.h

axml.o: axml.h
axml.o: globalVariables.h

bipartitionList.o: axml.h

classify.o: axml.h

evaluateGenericSpecial.o: axml.h

evaluatePartialGenericSpecial.o: axml.h

fastDNAparsimony.o: axml.h

fastSearch.o: axml.h

leaveDropping.o: axml.h

makenewzGenericSpecial.o: axml.h

mem_alloc.o: axml.h
mem_alloc.o: $(ARBHOME)/INCLUDE/malloc.h

models.o: axml.h

multiple.o: axml.h

newviewGenericSpecial.o: axml.h

optimizeModel.o: axml.h

parsePartitions.o: axml.h

rapidBootstrap.o: axml.h

rogueEPA.o: axml.h

searchAlgo.o: axml.h

topologies.o: axml.h

treeIO.o: axml.h
