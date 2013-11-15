# Variables we get from main makefile:
# DFLAGS: -D statements => add to preprocessor flags
CPPFLAGS += $(DFLAGS)
# CXX_INCLUDES:  -I statemenmts => add to g++ flags
CXXFLAGS += $(CXX_INCLUDES)
# ARBDB_LIB: -l statements => add to LIBS
LIBS += $(ARBDB_LIB)
# LIBPATH: -L statements => add to LDLIBS
LDLIBS += $(LIBPATH)
CXX=$(A_CXX) $(cflags) $(cxxflags)
CXXFLAGS += -fpic
CFLAGS += -fpic


#LANGUAGES = PERL PYTHON R
LANGUAGES = PYTHON

PYTHONINSTALLPATH=$(ARBHOME)/lib/python2.6
PERLINSTALLPATH=$(ARBHOME)/lib/perl5

INCLUDES = -I$(ARBHOME)/BINDINGS
CXXFLAGS += $(INCLUDES)
CFLAGS += $(INCLUDES)

export CPPFLAGS CXXFLAGS CFLAGS LIBS LDLIBS CXX 

ifeq ($(UNIT_TESTS),1)
TARGETS = $(LANGUAGES:%=%-test)
else
TARGETS = $(LANGUAGES:%=%-build)
endif

all: $(TARGETS)

clean: $(LANGUAGES:%=%-clean)
	-rm -f ARB_oolayer.o

ARB_oolayer.o: ARB_oolayer.h ARB_oolayer.cpp

PYTHON-build: ARB.i PYTHON/ARB.i
	cd PYTHON; ./setup.py build
	cd PYTHON; ./setup.py install --install-lib $(PYTHONINSTALLPATH) 

PYTHON-test: PYTHON-build
	cd PYTHON; ./test.py

PYTHON-clean:
	cd PYTHON; ./setup.py clean
	-rm -rf PYTHON/build
	-rm -rf $(PYTHONINSTALLPATH)
	-rm -f PYTHON/ARB.py PYTHON/ARB_wrap.cpp

PYTHON-test:



PERL-build: PERL/Makefile PERL/ARB_wrap.cpp
	$(MAKE) -C PERL install

PERL-clean: PERL/Makefile
	$(MAKE) -C PERL clean
	-rm -f PERL/Makefile.old PERL/ARB_wrap.cxx 
	-rm -rf $(PERLINSTALLPATH)

PERL/Makefile: PERL/Makefile.PL
	cd PERL; perl Makefile.PL

PERL/ARB_wrap.cxx: ARB.i PERL/ARB.i
	cd PERL; swig -c++ -perl ARB.i


