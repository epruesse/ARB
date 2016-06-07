
MENUS_TGT = $(ARBHOME)/lib/gde/arb.menu
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

all: $(MENUS_TGT) help $(CHECK_AS_EXPECTED) rm_obsolete

MENUS/%.genmenu : genmenu.header

MENUS/%.genmenu : MENUS/%.menu
		( cat genmenu.header; $(SED) -e "s/^[ ]*//" ) < $< > $@

MENUS_DIRTY=ARB_GDEmenus.dirty

$(MENUS_DIRTY) : $(MENUS_SRC) $(GENMENUS) Makefile $(PPP)
	$(PPP) -IMENUS $(MENUS_SRC) >$@

$(MENUS_TGT) : $(MENUS_DIRTY) $(CLEAN)
	chmod a+w $@ || true
	$(CLEAN) <$(MENUS_DIRTY) >$@
	rm $(MENUS_DIRTY)
	chmod a-w $@

# ------------------------------------------------------------

update_expected: $(MENUS_TGT)
	cp $(MENUS_TGT) $(MENUS_EXPECTED)

check_as_expected: $(MENUS_TGT) $(MENUS_EXPECTED)
	diff --unified --ignore-space-change --ignore-blank-lines  $(MENUS_EXPECTED) $(MENUS_TGT) 

# ------------------------------------------------------------

help: $(HELPFILELIST)
	@test -d $(GENHELPDEST) || mkdir $(GENHELPDEST)
	@test -d $(GENDOCDEST) || mkdir $(GENDOCDEST)
	@$(MAKE) -f Makefile.helpfiles "GENHELPDEST=$(GENHELPDEST)" "GENDOCDEST=$(GENDOCDEST)" "HELPFILELIST=$(HELPFILELIST)"

$(HELPFILELIST): $(SUBMENUS) Makefile
	grep -ih '^itemhelp:' $(SUBMENUS) | $(SED) -e 's/^itemhelp://' | sort | uniq > $@

# ------------------------------------------------------------

rm_obsolete:
	@(test -f ARB_GDEmenus && rm -f ARB_GDEmenus) || true

clean: $(HELPFILELIST)
	@$(MAKE) -f Makefile.helpfiles \
		"GENHELPDEST=$(GENHELPDEST)" \
		"GENDOCDEST=$(GENDOCDEST)" \
		"HELPFILELIST=$(HELPFILELIST)" \
		clean
	rm -f $(GENMENUS) $(MENUS_TGT) $(HELPFILELIST)

