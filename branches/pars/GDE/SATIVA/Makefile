# Wrapper to build RAxML versions

# _PORTABLE_PTHREADS in axml.c to get pthread pinning on linux
# use approx 1 thread per 500bp

# Versions of RAxML to build
VERSIONS:=SSE3.PTHREADS PTHREADS 

# AVX needs gcc >= 4.6.0 
ifeq ("$(USE_GCC_46_OR_HIGHER)", "yes")
  VERSIONS+=AVX.PTHREADS
endif

TARFILE=$(wildcard sativa-RAxML-8.*.*gz)

ifeq (($DARWIN),1)
  MAKE_SUFFIX=mac	
else
  MAKE_SUFFIX=gcc
endif

RAXML_BIN=$(ARBHOME)/bin/raxmlHPC8-sativa-

## targets called by "upsteam makefile":

all: install

clean_local:
	rm -rf builddir.*
	rm -f $(RAXML_BIN)*
	rm -f unpack.*.stamp

## internal targets

install: build
	cp arb_sativa.pl $(ARBHOME)/bin
	chmod a+x $(ARBHOME)/bin/arb_sativa.pl
	cp arb_raxml8_sativa.sh $(ARBHOME)/bin
	chmod a+x $(ARBHOME)/bin/arb_raxml8_sativa.sh
	cp sativa.menu $(ARBHOME)/lib/gde
	find sativa -path "*.svn" -prune -o -type d -exec mkdir -p $(ARBHOME)/lib/{} \; 
	find sativa -path "*.svn" -prune -o -type f -exec cp -p {} $(ARBHOME)/lib/{} \; 
#	ln -s $(ARBHOME)/bin $(ARBHOME)/bin/sativa/epac/bin

uninstall:
	rm -f $(ARBHOME)/bin/arb_sativa.pl
	rm -f $(ARBHOME)/bin/arb_raxml8_sativa.sh
	rm -rf $(ARBHOME)/lib/sativa
	rm -f $(ARBHOME)/lib/gde/sativa.menu

clean: clean_local uninstall

build:  $(VERSIONS:%=$(RAXML_BIN)%)

# (MAKEFLAGS need to be unset as ARB sets "-r", suppressing built-in rules,
#  which are needed by RAxML)

$(RAXML_BIN)%: unpack.%.stamp
	MAKEFLAGS= $(MAKE) -C $(<:unpack.%.stamp=builddir.%) -f Makefile.$(@:$(RAXML_BIN)%=%).$(MAKE_SUFFIX)
	cp $(<:unpack.%.stamp=builddir.%)/raxmlHPC-* $@

unpack.%.stamp: $(TARFILE)
	(DIR=$(@:unpack.%.stamp=builddir.%); \
	rm -rf $$DIR &&\
	mkdir $$DIR &&\
	tar -C $$DIR --strip-components=1 -xzf $(TARFILE) &&\
	arb_sed -i 's/-march=native//' $$DIR/Makefile*)
	touch $@ -r $(TARFILE)	

%.stamp:
	echo $@

# prevent make from deleting intermediate targets:
.SECONDARY:

.PHONY: clean all install build uninstall clean_local
