
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
ifeq ($(SANITIZE),1)
	@echo "Warning: cannot test perl scripts (atm) if SANITIZE is active" # @@@ still fails -- accept atm
	@true
else
	@perl -c testScripts.pl
	@perl testScripts.pl
	@touch $@
endif

clean:
	rm test.stamp
