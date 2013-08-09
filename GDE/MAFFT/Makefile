# MAFFT for ARB
# by Arne Böckmann (2013)

PROG  = MAFFT

# force gcc (hack to fix cent5 build)
export CC = gcc

#disable makeflags
MAKEFLAGS=

# ARBs standard target
$(MAIN) : mafft.stamp

clean :
	rm -rf $(ARBHOME)/lib/mafft
	rm -f mafft.stamp
	rm -f $(ARBHOME)/bin/mafft*
	cd mafft-7.055-with-extensions/core && make clean
	cd mafft-7.055-with-extensions/extensions && make clean

mafft.stamp :
	cd mafft-7.055-with-extensions/core && make && make install
	cd mafft-7.055-with-extensions/extensions && make && make install
	touch mafft.stamp

.PHONY : clean
