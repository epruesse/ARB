help.make:
	@echo	'options'
	@echo	'	slib			create links to perl, Xlib'
	@echo	'	smacro			set permissions in the macros dir'
	@echo	'	shelp			set permissions in the help dir'
	@echo	'	'
	@echo	'	all				all above'
	@echo	'	'
	@echo	'	nameserver		create nameserver from template'
	@echo	'	msg				edit daily arb message'

all:	slib smacro shelp

slib:
	rm -f dummyprg `find * -type l ! -name pts -print -o -type d -prune`
	-mkdir ../Xlib
	(ln -f -s `find ../Xlib -print` . || echo 'using standard X11 libs')
	ln -f -s ../DEPOT/ghostscript .
	ln -f -s ../PERL5/perl5 .

smacro:
	chmod -R g+w macros

shelp:
	(cd help; make all)


nameserver:
		(cd nas;$(MAKE) all)

msg:	$(HOME)/.arb_prop/time

$(HOME)/.arb_prop/time: $(ARBHOME)/lib/message
	touch $@
	arb_textedit $(ARBHOME)/lib/message

