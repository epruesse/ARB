
TARGET = ARB_GDEmenus
SOURCE = ARB_GDEmenus.source
HELPFILELIST=helpfiles.lst
GENHELPDEST=./HELP_GEN
GENDOCDEST=./HELP_DOC_GEN

MENUS=$(wildcard MENUS/*.menu)
GENMENUS=$(MENUS:.menu=.genmenu)

all: $(TARGET) help

MENUS/%.genmenu : genmenu.header

MENUS/%.genmenu : MENUS/%.menu
		( cat genmenu.header; sed -e "s/^[ ]*//" ) < $< > $@

$(TARGET) : $(SOURCE) $(GENMENUS)
	-chmod a+w $(TARGET)
	$(PP)  -IMENUS $(SOURCE) >$(TARGET)
	chmod a-w $(TARGET)

# ------------------------------------------------------------

help: $(HELPFILELIST)
	@test -d $(GENHELPDEST) || mkdir $(GENHELPDEST)
	@test -d $(GENDOCDEST) || mkdir $(GENDOCDEST)
	@$(MAKE) -f Makefile.helpfiles "GENHELPDEST=$(GENHELPDEST)" "GENDOCDEST=$(GENDOCDEST)" "HELPFILELIST=$(HELPFILELIST)"

$(HELPFILELIST): $(TARGET)
	grep -i '^itemhelp:' < $< | sed -e 's/^itemhelp://' | sort | uniq > $@

# ------------------------------------------------------------

save:
	rm -f arbgdemenus*.tar.gz
	tar cvf - MENUS ARB_GDEmenus* |gzip >`date '+arbgdemnus_%d_%m_%y.tar.gz'`

clean:
	-rm -f $(GENMENUS) $(TARGET) $(HELPFILELIST)
	@$(MAKE) -f Makefile.helpfiles "GENHELPDEST=$(GENHELPDEST)" "GENDOCDEST=$(GENDOCDEST)" "HELPFILELIST=$(HELPFILELIST)" clean

