
TARGET = ARB_GDEmenus
SOURCE = ARB_GDEmenus.source
DEPEND_ON = $(wildcard MENUS/*.menu)

all: $(TARGET)

$(TARGET) : $(SOURCE) $(DEPEND_ON)
	chmod a+w $(TARGET)
	$(PP)  -IMENUS $(SOURCE) >$(TARGET)
	chmod a+w $(TARGET)

save:
	rm -f arbgdemenus*.tar.gz
	tar cvf - MENUS ARB_GDEmenus* |gzip >`date '+arbgdemnus_%d_%m_%y.tar.gz'`
