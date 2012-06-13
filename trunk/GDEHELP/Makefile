
MENUS = ARB_GDEmenus
MENUS_SRC = ARB_GDEmenus.source

HELPFILELIST=helpfiles.lst
GENHELPDEST=./HELP_GEN
GENDOCDEST=./HELP_DOC_GEN

SUBMENUS=$(wildcard MENUS/*.menu)
GENMENUS=$(SUBMENUS:.menu=.genmenu)

PPP:=./pp.pl

all: $(MENUS) help

MENUS/%.genmenu : genmenu.header

MENUS/%.genmenu : MENUS/%.menu
		( cat genmenu.header; sed -e "s/^[ ]*//" ) < $< > $@

$(MENUS) : $(MENUS_SRC) $(GENMENUS) Makefile
	-chmod a+w $@
	$(PPP) -IMENUS $(MENUS_SRC) >$@
	chmod a-w $@

# ------------------------------------------------------------

help: $(HELPFILELIST)
	@test -d $(GENHELPDEST) || mkdir $(GENHELPDEST)
	@test -d $(GENDOCDEST) || mkdir $(GENDOCDEST)
	@$(MAKE) -f Makefile.helpfiles "GENHELPDEST=$(GENHELPDEST)" "GENDOCDEST=$(GENDOCDEST)" "HELPFILELIST=$(HELPFILELIST)"

$(HELPFILELIST): $(SUBMENUS)
	grep -i '^itemhelp:' < $< | sed -e 's/^itemhelp://' | sort | uniq > $@

# ------------------------------------------------------------

save:
	rm -f arbgdemenus*.tar.gz
	tar cvf - MENUS ARB_GDEmenus* |gzip >`date '+arbgdemnus_%d_%m_%y.tar.gz'`

clean: $(HELPFILELIST)
	@$(MAKE) -f Makefile.helpfiles \
		"GENHELPDEST=$(GENHELPDEST)" \
		"GENDOCDEST=$(GENDOCDEST)" \
		"HELPFILELIST=$(HELPFILELIST)" \
		clean
	rm -f $(GENMENUS) $(MENUS) $(HELPFILELIST)

