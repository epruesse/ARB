
TARGET = ARB_GDEmenus
SOURCE = ARB_GDEmenus.source

MENUS=$(wildcard MENUS/*.menu)
GENMENUS=$(MENUS:.menu=.genmenu)

all: $(TARGET)

MENUS/%.genmenu : MENUS/%.menu
		sed -e "s/^[ ]*//ig" < $< > $@

$(TARGET) : $(SOURCE) $(GENMENUS)
	-chmod a+w $(TARGET)
	$(PP)  -IMENUS $(SOURCE) >$(TARGET)
	chmod a-w $(TARGET)

save:
	rm -f arbgdemenus*.tar.gz
	tar cvf - MENUS ARB_GDEmenus* |gzip >`date '+arbgdemnus_%d_%m_%y.tar.gz'`
