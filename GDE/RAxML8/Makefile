# Wrapper to build RAxML versions

# _PORTABLE_PTHREADS in axml.c to get pthread pinning on linux
# use approx 1 thread per 500bp

# Versions of RAxML to build
VERSIONS:=SSE3.PTHREADS PTHREADS 

# AVX needs gcc >= 4.6.0 
ifeq ("$(USE_GCC_46_OR_HIGHER)", "yes")
  VERSIONS+=AVX.PTHREADS
endif

TARFILE=$(wildcard standard-RAxML-8.*.*gz)

ifeq ($(DARWIN),1)
  MAKE_SUFFIX=mac	
else
  MAKE_SUFFIX=gcc
endif

RAXML_BIN=$(ARBHOME)/bin/raxmlHPC8-

## targets called by "upsteam makefile":

all: install

clean:
	rm -rf builddir.*
	rm -f $(RAXML_BIN)*
	rm -f unpack.*.stamp

## internal targets

install: build
	cp arb_raxml8.sh $(ARBHOME)/bin
	chmod a+x $(ARBHOME)/bin/arb_raxml8.sh
	cp raxml8.menu $(ARBHOME)/lib/gde

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

.PHONY: clean all
