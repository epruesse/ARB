help:
	@echo	'Known targets:'
	@echo	''
	@echo	'  all		create all links to binaries in: SH PERL_SCRIPTS'
	@echo	'  develall     create links to arb_binaries in: SOURCE_TOOLS (development only)'
	@echo	'  realclean:   remove all executables and links'
	@echo	'  cleanlinks:  remove all links'

all: cleanlinks
	ln -s -f `find ../SH ../PERL_SCRIPTS -perm -100 ! -type d ! -name 'config*' -print` .
	ln -s mafft mafft-linsi
	ln -s mafft mafft-ginsi
	ln -s mafft mafft-fftns
	ln -s mafft mafft-fftnsi
	ln -s mafft mafft-nwns
	ln -s mafft mafft-nwnsi
	ln -s mafft mafft-einsi
	ln -s mafft mafft-xinsi
	ln -s mafft mafft-qinsi

develall: all
	ln -s -f `find ../SOURCE_TOOLS -name "arb_*" -perm -100 ! -type d -print` .


clean: realclean

realclean:
	-find . -type l            -exec rm {} \;
	-find . -type f -perm -100 -exec rm {} \;


cleanlinks:
	-find . -type l -exec rm {} \;
