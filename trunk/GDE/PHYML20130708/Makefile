# PHYML 20130708 Makefile for ARB
# by Arne Böckmann (2013)

PROG  = PHYML_20130708


# ARBs standard target
$(MAIN) : phy


clean :
	rm -rf build && \
	rm -f $(ARBHOME)/bin/phyml_20130708

phy : 
	mkdir -p build && \
	cd build && \
	../phyml/configure CPP='' --prefix=$(ARBHOME) --program-suffix=_20130708 && \
	make && make install

.PHONY : clean phy