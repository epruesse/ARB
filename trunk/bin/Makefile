help:
	@echo	'Options'
	@echo	'	all:	Create all links to binaries in:'
	@echo	'		DEPOT SH PERL_SCRIPTS'
	@echo	'	realclean: remove everything but this Makefile'
	@echo	'	arbclean:  remove all arb_binaries'


all:
	rm -f dummyprg `find . -type l -print`
ifndef DEBIAN
	/bin/ln -s `find ../DEPOT ../SH ../PERL5/bin ../PERL_SCRIPTS -perm -100 ! -type d ! -name 'config*' -print` .
else
	/bin/ln -s `find ../SH ../PERL_SCRIPTS -perm -100 ! -type d ! -name 'config*' -print` .
endif

realclean:
	rm -f dummyprg `find . ! -name Makefile -print`

arbclean:
	rm -f `find . ! -type l -name 'arb_*' -print`
