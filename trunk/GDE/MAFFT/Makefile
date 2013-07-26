# MAFFT for ARB
# by Arne Böckmann (2013)

PROG  = MAFFT

# force gcc (hack to fix cent5 build)
export CC = gcc

# ARBs standard target
$(MAIN) : mafft.stamp

clean :
	rm -rf $(ARBHOME)/lib/mafft
	rm -f mafft.stamp
	rm -f $(ARBHOME)/bin/mafft
	cd mafft-7.053-without-extensions/core && make clean

mafft.stamp :
	cd mafft-7.053-without-extensions/core && make && make install
	touch mafft.stamp

.PHONY : clean
