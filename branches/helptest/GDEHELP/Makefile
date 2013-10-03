
MENUS = ARB_GDEmenus
MENUS_SRC = ARB_GDEmenus.source

SED:=$(ARBHOME)/SH/arb_sed

HELPFILELIST=helpfiles.lst
GENHELPDEST=./HELP_GEN
GENDOCDEST=./HELP_DOC_GEN

SUBMENUS=$(wildcard MENUS/*.menu)
GENMENUS=$(SUBMENUS:.menu=.genmenu)

PPP:=./pp.pl
CLEAN:=./cleanmenu.pl

# ---------------------------------------- help for refactoring
MENUS_EXPECTED = ARB_GDEmenus.expected

# check expected result (to avoid unwanted changes)
# call 'make update_expected' to expect the current result

#CHECK_AS_EXPECTED = check_as_expected
CHECK_AS_EXPECTED = 

# ----------------------------------------

all: $(MENUS) help $(CHECK_AS_EXPECTED)

MENUS/%.genmenu : genmenu.header

MENUS/%.genmenu : MENUS/%.menu
		( cat genmenu.header; $(SED) -e "s/^[ ]*//" ) < $< > $@

MENUS_DIRTY=$(MENUS).dirty

$(MENUS_DIRTY) : $(MENUS_SRC) $(GENMENUS) Makefile $(PPP)
	$(PPP) -IMENUS $(MENUS_SRC) >$@

$(MENUS) : $(MENUS_DIRTY) $(CLEAN)
	-chmod a+w $@
	$(CLEAN) <$(MENUS_DIRTY) >$@
	rm $(MENUS_DIRTY)
	chmod a-w $@

# ------------------------------------------------------------

update_expected: $(MENUS)
	cp $(MENUS) $(MENUS_EXPECTED)

check_as_expected: $(MENUS) $(MENUS_EXPECTED)
	diff --unified --ignore-space-change --ignore-blank-lines  $(MENUS_EXPECTED) $(MENUS) 

# ------------------------------------------------------------

help: $(HELPFILELIST)
	@test -d $(GENHELPDEST) || mkdir $(GENHELPDEST)
	@test -d $(GENDOCDEST) || mkdir $(GENDOCDEST)
	@$(MAKE) -f Makefile.helpfiles "GENHELPDEST=$(GENHELPDEST)" "GENDOCDEST=$(GENDOCDEST)" "HELPFILELIST=$(HELPFILELIST)"

$(HELPFILELIST): $(SUBMENUS) Makefile
	grep -ih '^itemhelp:' $(SUBMENUS) | $(SED) -e 's/^itemhelp://' | sort | uniq > $@

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

