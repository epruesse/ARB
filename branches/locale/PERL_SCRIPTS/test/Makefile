
LIBDIR := $(ARBHOME)/lib

ifeq ($(DARWIN),1)
SO:=bundle
else
SO:=so
endif

ARB_PM := $(LIBDIR)/ARB.pm
ARB_SO := $(LIBDIR)/ARB.$(SO)

help:
	@echo "test      tests all perl scripts for syntax"

test: test.stamp

test.stamp: $(ARB_PM) $(ARB_SO)
	@echo $(SEP) Testing scripts in PERL_SCRIPTS
	@perl -c testScripts.pl
	@perl testScripts.pl
	@touch $@

clean:
	rm test.stamp
