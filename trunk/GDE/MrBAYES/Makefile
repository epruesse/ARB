# MrBayes Makefile for ARB
# by Arne Boeckmann (2013)

PROG   = MrBAYES
SUB    = mrbayes_3.2.1
FOLDER =$(ARBHOME)/GDE/MrBAYES/$(SUB)

LOCBIN=$(FOLDER)/mb
LOCDEPS=$(FOLDER)/Makefile $(wildcard $(FOLDER)/*.c) $(wildcard $(FOLDER)/*.h)

INSTBIN=$(ARBHOME)/bin/mb

CC:=$(A_CC)
CXX:=$(A_CXX)

# ARBs standard target
$(MAIN) : $(INSTBIN)

clean :
	rm -f $(INSTBIN)
	cd $(FOLDER) && $(MAKE) clean

$(INSTBIN) : $(LOCBIN)
	cp $(LOCBIN) $(INSTBIN)

$(LOCBIN): $(LOCDEPS)
	+cd $(FOLDER) && $(MAKE) mb

depends: $(SUB)/$(SUB).depend

%.depend:
	@cp -p $(@D)/Makefile $(@D)/Makefile.old # save old Makefile
	@$(MAKE) -C $(@D) -r depends
	@grep "^# DO NOT DELETE" $(@D)/Makefile >/dev/null || \
		(echo "$(@D)/Makefile:666: Error: No dependencies in $(@D)/Makefile"; false) # check whether sub Makefile has dependencies
	@$(ARBHOME)/SOURCE_TOOLS/fix_depends.pl "(from GDE/MrBayes)" < $(@D)/Makefile > $(@D)/Makefile.2
	@mv $(@D)/Makefile.old $(@D)/Makefile # restore old Makefile
	@$(ARBHOME)/SOURCE_TOOLS/mv_if_diff $(@D)/Makefile.2 $(@D)/Makefile # update Makefile if changed

.PHONY : clean depends

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from GDE)
