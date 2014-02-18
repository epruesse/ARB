# MrBayes Makefile for ARB
# by Arne Boeckmann (2013)

PROG  = MrBAYES
FOLDER =$(ARBHOME)/GDE/MrBAYES/mrbayes_3.2.1

LOCBIN=$(FOLDER)/mb
LOCDEPS=$(FOLDER)/Makefile $(wildcard $(FOLDER)/*.c) $(wildcard $(FOLDER)/*.h)

INSTBIN=$(ARBHOME)/bin/mb

# ARBs standard target
$(MAIN) : $(INSTBIN)

clean :
	rm -f $(INSTBIN)
	cd $(FOLDER) && make clean

$(INSTBIN) : $(LOCBIN)
	cp $(LOCBIN) $(INSTBIN)

$(LOCBIN): $(LOCDEPS)
	+cd $(FOLDER) && $(MAKE) mb

.PHONY : autounstamp clean

