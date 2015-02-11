# PHYML 20130708 Makefile for ARB
# by Arne Boeckmann (2013)

SUFFIX   = _20130708
PROG     = PHYML$(SUFFIX)
BUILDDIR = build

INSTALLED_BINARY = $(ARBHOME)/bin/phyml$(SUFFIX)
CONFIG_STATUS    = $(BUILDDIR)/config.status

# automatically remove now obsolete file (@@@ remove this code after some time)
OBSOLETE_STAMP   = phyml.stamp

CC=$(A_CC)
CXX=$(A_CXX)

# ARBs standard target:
$(MAIN) : $(INSTALLED_BINARY)
	rm -f $(OBSOLETE_STAMP)
ifeq ($(DONT_RETRY_TWICE),1)
	test -f $(INSTALLED_BINARY)
else
	test -f $(INSTALLED_BINARY) || $(MAKE) retry
endif

retry: clean_build
	@echo "---------------------------------------- PHYML 'make install' failed to install -> trigger rebuild"
	$(MAKE) "DONT_RETRY_TWICE=1" $(MAIN)
	@echo "---------------------------------------- PHYML rebuild succeeded"

$(INSTALLED_BINARY): $(CONFIG_STATUS)
	$(MAKE) -C $(BUILDDIR) install

phyml/configure : phyml/configure.fallback
	@echo "Using $<:"
	cp -p $< $@

phyml/config.h.in : phyml/config.h.in.fallback
	@echo "Using $<:"
	cp -p $< $@

$(CONFIG_STATUS): Makefile phyml/configure phyml/config.h.in
	$(MAKE) clean_build
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && ../phyml/configure CPP='' --prefix=$(ARBHOME) --program-suffix=$(SUFFIX)

clean_build:
	rm -rf $(BUILDDIR)
	rm -f $(INSTALLED_BINARY)

clean : clean_build
	rm -f phyml/config.h.in
	rm -f phyml/configure


.PHONY : clean clean_build

