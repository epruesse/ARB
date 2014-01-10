# fasttree2 Makefile for ARB
# by Arne Boeckmann (2013)

PROG  = FASTTREE

CC ?= gcc

#WFLAGS=-Wall
WFLAGS=-w

# ARBs standard target
$(MAIN) : FastTree

FastTree:
	$(CC) -O3 -finline-functions -funroll-loops $(WFLAGS) -o FastTree FastTree.c -lm
	cp -f FastTree $(ARBHOME)/bin

clean :
	rm -f FastTree
	rm -f $(ARBHOME)/bin/FastTree

.PHONY : clean


