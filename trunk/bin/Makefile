help:
	@echo	'Known targets:'
	@echo	''
	@echo	'  all		create all links to binaries in: SH PERL_SCRIPTS'
	@echo	'  develall     create links to arb_binaries in: SOURCE_TOOLS (development only)'
	@echo	'  realclean:   remove all executables and links'
	@echo	'  cleanlinks:  remove all links'

all: cleanlinks
	ln -s -f `find ../SH ../PERL_SCRIPTS -perm -100 ! -type d ! -name 'config*' -print` .

develall: all
	ln -s -f `find ../SOURCE_TOOLS -name "arb_*" -perm -100 ! -type d -print` .


clean: realclean

realclean:
	find . -type l            -exec rm {} \;
	find . -type f -perm -100 -exec rm {} \;


cleanlinks:
	find . -type l -exec rm {} \;
