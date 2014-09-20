
.PHONY: help all develall mafftlinks scriptlinks devellinks clean cleanbinaries cleanlinks

help:
	@echo 'Known targets:'
	@echo ''
	@echo '  all          create all links to binaries in: SH PERL_SCRIPTS'
	@echo '  develall     create links to arb_binaries in: SOURCE_TOOLS (development only)'
	@echo '  clean:       remove all executables and links'
	@echo '  cleanlinks:  remove all links'

# ----------------------------------------

all: cleanlinks
	$(MAKE) scriptlinks mafftlinks

develall: cleanlinks
	$(MAKE) scriptlinks mafftlinks devellinks

# ----------------------------------------

MAFFTLINKS= \
	mafft-einsi \
	mafft-fftns \
	mafft-fftnsi \
	mafft-ginsi \
	mafft-linsi \
	mafft-nwns \
	mafft-nwnsi \
	mafft-qinsi \
	mafft-xinsi \

mafft-%:
	ln -s -f mafft $@

mafftlinks: $(MAFFTLINKS)

scriptlinks:
	ln -s -f `find ../SH           -perm -100 ! -type d -print` .
	ln -s -f `find ../PERL_SCRIPTS -perm -100 ! -type d ! -path '*/IFTHELP/*' ! -path '*/TESTS/*' -print` .

devellinks:
	ln -s -f `find ../SOURCE_TOOLS -name "arb_*" -perm -100 ! -type d -print` .

# ----------------------------------------
clean: cleanlinks cleanbinaries

cleanbinaries:
	-find . -type f -perm -100 -exec rm {} \;

cleanlinks:
	-find . -type l -exec rm {} \;
