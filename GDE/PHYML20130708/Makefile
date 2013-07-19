# PHYML 20130708 Makefile for ARB
# by Arne Böckmann (2013)

PROG  = PHYML_20130708


# ARBs standard target
$(MAIN) : phyml.stamp


clean :
	rm -rf build
	rm -f phyml.stamp
	rm -f $(ARBHOME)/bin/phyml_20130708



build/config.status:
	mkdir -p build && \
	cd build && \
        ../phyml/configure CPP='' --prefix=$(ARBHOME) --program-suffix=_20130708

phyml.stamp : build/config.status
	cd build && make && make install
	touch phyml.stamp

	

.PHONY : clean


	