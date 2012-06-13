
MENUS = ARB_GDEmenus
MENUS_NEW = ARB_GDEmenus.new
MENUS_SRC = ARB_GDEmenus.source

HELPFILELIST=helpfiles.lst
GENHELPDEST=./HELP_GEN
GENDOCDEST=./HELP_DOC_GEN

SUBMENUS=$(wildcard MENUS/*.menu)
GENMENUS=$(SUBMENUS:.menu=.genmenu)

#PP:=cpp
PPP:=./pp.pl
CLEAN:=./cleanmenu.pl

all: compare_menus $(MENUS) help

MENUS/%.genmenu : genmenu.header

MENUS/%.genmenu : MENUS/%.menu
		( cat genmenu.header; sed -e "s/^[ ]*//" ) < $< > $@

$(MENUS) : $(MENUS_SRC) $(GENMENUS) Makefile $(CLEAN)
	-chmod a+w $@
	$(PP)  -IMENUS $(MENUS_SRC) | $(CLEAN) >$@
	chmod a-w $@

$(MENUS_NEW) : $(MENUS_SRC) $(GENMENUS) $(PPP) Makefile $(CLEAN)
	-chmod a+w $@
	$(PPP) -IMENUS $(MENUS_SRC) | $(CLEAN) >$@
	chmod a-w $@

compare_menus: $(MENUS) $(MENUS_NEW)
	diff -w -b -B -E $(MENUS) $(MENUS_NEW)
	@echo "No difference between generated menus"
#	diff -w -b -B -E $(MENUS) $(MENUS_NEW) | head -80
	-rm -f $(MENUS_NEW)

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
	rm -f $(GENMENUS) $(MENUS) $(MENUS_NEW) $(HELPFILELIST)

