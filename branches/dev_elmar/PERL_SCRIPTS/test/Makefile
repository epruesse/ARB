
LIBDIR := $(ARBHOME)/lib

ifdef DARWIN
SO:=bundle
else
SO:=so
endif

ARB_PM := $(LIBDIR)/ARB.pm
ARB_SO := $(LIBDIR)/ARB.$(SO)

help:
	test		test all perl scripts for syntax

test: test.stamp

test.stamp: $(ARB_PM) $(ARB_SO)
	@echo $(SEP) Testing scripts in PERL_SCRIPTS
	@perl -c testScripts.pl
	@perl testScripts.pl || echo "You either lack S::LintSubs and/or Bio::TreeIO or the perl scripts are buggy. We continue anyway.."
	@touch $@

clean:
	rm test.stamp
