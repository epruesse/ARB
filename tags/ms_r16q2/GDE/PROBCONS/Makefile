# Probcons for ARB
# by Arne Boeckmann (2013)

PROG   = PROBCONS
TARGET = $(ARBHOME)/bin/probcons


# ARBs standard target
$(MAIN) : $(TARGET)

clean :
	+cd probcons && make clean
	rm -f $(TARGET)
	rm -f probcons.stamp

$(TARGET) : probcons.stamp
	cp -f probcons/probcons $(TARGET)

probcons.stamp:
	+cd probcons && make
	touch probcons.stamp

.PHONY : clean
