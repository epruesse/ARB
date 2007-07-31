help:
	@echo	'Known targets:'
	@echo	''
	@echo	'  all		create all links to binaries in: SH PERL_SCRIPTS'
	@echo	'  develall     create links to arb_binaries in: SOURCE_TOOLS (development only)'
	@echo	'  realclean:   remove everything but this Makefile'
	@echo	'  cleanlinks:  remove all links'

all: cleanlinks
	/bin/ln -s `find ../SH ../PERL_SCRIPTS -perm -100 ! -type d ! -name 'config*' -print` .

develall: all
	/bin/ln -s `find ../SOURCE_TOOLS -name "arb_*" -perm -100 ! -type d -print` .


realclean:
	rm -f dummyprg `find . ! -name Makefile -print`

cleanlinks:
	ln -s dummy dummy
	rm -f dummy `find . -type l -print`
