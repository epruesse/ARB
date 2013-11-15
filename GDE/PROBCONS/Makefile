# Probcons for ARB
# by Arne Boeckmann (2013)

PROG  = PROBCONS


# ARBs standard target
$(MAIN) : probcons.stamp

clean :
	+cd probcons && make clean
	rm -f $(ARBHOME)/bin/probcons
	rm -f probcons.stamp


probcons.stamp :
	+cd probcons && make
	cp -f probcons/probcons $(ARBHOME)/bin/probcons
	touch probcons.stamp

.PHONY : clean
