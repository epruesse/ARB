help:
	@echo	'Options'
	@echo	'	all:	Create all links to binaries in: SH PERL_SCRIPTS'
#	@echo	'		DEPOT SH PERL_SCRIPTS'
	@echo	'	realclean: remove everything but this Makefile'
	@echo	'	arbclean:  remove all arb_binaries'


all:
	rm -f dummyprg `find . -type l -print`
ifdef DEBIAN
	/bin/ln -s `find ../SH ../PERL_SCRIPTS -perm -100 ! -type d ! -name 'config*' -print` .
else
	/bin/ln -s `find ../SH ../PERL_SCRIPTS -perm -100 ! -type d ! -name 'config*' -print` .
#	/bin/ln -s `find ../DEPOT ../SH ../PERL5/bin ../PERL_SCRIPTS -perm -100 ! -type d ! -name 'config*' -print` .
endif
	/bin/ln -s `find ../SOURCE_TOOLS -name "arb_*" -perm -100 ! -type d -print` .


realclean:
	rm -f dummyprg `find . ! -name Makefile -print`

arbclean:
	rm -f `find . ! -type l -name 'arb_*' -print`
