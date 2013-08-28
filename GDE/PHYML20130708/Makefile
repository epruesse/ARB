# PHYML 20130708 Makefile for ARB
# by Arne Böckmann (2013)

SUFFIX   = _20130708
PROG     = PHYML$(SUFFIX)
BUILDDIR = build

INSTALLED_BINARY = $(ARBHOME)/bin/phyml$(SUFFIX)
CONFIG_STATUS    = $(BUILDDIR)/config.status

# automatically remove now obsolete file (@@@ remove this code after some time)
OBSOLETE_STAMP   = phyml.stamp

# ARBs standard target:
$(MAIN) : $(INSTALLED_BINARY)
	rm -f $(OBSOLETE_STAMP)
	test -f $(INSTALLED_BINARY)

$(INSTALLED_BINARY): $(CONFIG_STATUS)
	$(MAKE) -C $(BUILDDIR) install

$(CONFIG_STATUS): Makefile
	$(MAKE) clean
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && ../phyml/configure CPP='' --prefix=$(ARBHOME) --program-suffix=$(SUFFIX)

clean :
	rm -rf $(BUILDDIR)
	rm -f $(INSTALLED_BINARY)

.PHONY : clean 

