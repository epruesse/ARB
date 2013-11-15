# MAFFT for ARB
# by Arne Boeckmann (2013)

PROG  = MAFFT

# force gcc (hack to fix cent5 build)
export CC = gcc

#disable makeflags
MAKEFLAGS=

# ARBs standard target
$(MAIN) : mafft.stamp

clean_core:
	+cd mafft-7.055-with-extensions/core && make clean

clean_extensions:
	+cd mafft-7.055-with-extensions/extensions && make clean

clean_here:
	rm -rf $(ARBHOME)/lib/mafft
	rm -f mafft.stamp
	rm -f $(ARBHOME)/bin/mafft*

clean: clean_core clean_extensions clean_here

make_core:
	+cd mafft-7.055-with-extensions/core && make && make install

make_extensions:
	+cd mafft-7.055-with-extensions/extensions && make && make install

mafft.stamp: make_core make_extensions
	touch mafft.stamp

.PHONY : clean clean_core clean_extensions clean_here make_core make_extensions
