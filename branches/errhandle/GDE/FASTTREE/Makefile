# fasttree2 Makefile for ARB
# by Arne Boeckmann (2013)

PROG  = FASTTREE

CC=$(A_CC)

#WFLAGS=-Wall
WFLAGS=-w

SOURCE=FastTree.c
LOCBIN=FastTree
INSTBIN=$(ARBHOME)/bin/FastTree

# ARBs standard target
$(MAIN) : $(INSTBIN)

$(INSTBIN) : $(LOCBIN)
	cp -f $(LOCBIN) $(INSTBIN)

$(LOCBIN): Makefile $(SOURCE)
	$(CC) -O3 -finline-functions -funroll-loops $(WFLAGS) -o $(LOCBIN) $(SOURCE) -lm

clean :
	rm -f $(LOCBIN) $(INSTBIN)

.PHONY : clean


