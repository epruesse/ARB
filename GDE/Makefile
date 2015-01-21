
ARCHS_COMMON_DEPENDS = \
	CLUSTAL/CLUSTAL.dummy  \
	SUPPORT/SUPPORT.dummy \
	AxML/AxML.dummy \

ARCHS_COMMON_NO_DEPENDS = \
	PHYML20130708/PHYML20130708.dummy \
	RAxML8/RAxML8.dummy \
	SATIVA/SATIVA.dummy \


# --------------------------------------------------------------------------------
# If a package is available as debian package it should go here
# 
# Note: I'm not sure whether this is a good idea. Due to limited manpower it often takes
# longer until ARB works with newer (incompatible) versions of these tools. --ralf

ARCHS_NON_DEBIAN_DEPENDS = \
	CLUSTALW/CLUSTALW.dummy  \
	FASTDNAML/FASTDNAML.dummy \
	MrBAYES/MrBAYES.dummy \
	PHYLIP/PHYLIP.dummy \
	PHYML/PHYML.dummy \
	RAxML/RAxML.dummy \
	TREEPUZZLE/TREEPUZZLE.dummy \

ARCHS_NON_DEBIAN_NO_DEPENDS = \
	FASTTREE/FASTTREE.dummy \
	MAFFT/MAFFT.dummy \
	MUSCLE/MUSCLE.dummy \
	PROBCONS/PROBCONS.dummy \

# --------------------------------------------------------------------------------

ifeq ($(DEBIAN),1)
ARCHS_DEPENDS    = $(ARCHS_COMMON_DEPENDS)
ARCHS_NO_DEPENDS = $(ARCHS_COMMON_NO_DEPENDS)
else
ARCHS_DEPENDS    = $(ARCHS_COMMON_DEPENDS)    $(ARCHS_NON_DEBIAN_DEPENDS)
ARCHS_NO_DEPENDS = $(ARCHS_COMMON_NO_DEPENDS) $(ARCHS_NON_DEBIAN_NO_DEPENDS)
endif

ARCHS=$(ARCHS_DEPENDS) $(ARCHS_NO_DEPENDS)

# --------------------------------------------------------------------------------
# modify cflags for submakefiles 

RAISE_WARNINGS=0# no warnings in this subtree
UNIT_TESTS=0# no tests in this subtree

sub_cflags:=$(cflags)

ifeq ('$(RAISE_WARNINGS)','0')
sub_cflags:=$(subst -W -Wall,-w,$(sub_cflags))
endif
ifeq ('$(UNIT_TESTS)','0')
sub_cflags:=$(subst -DUNIT_TESTS,,$(sub_cflags))
endif

# note the above does not affect all subdirs. see also manual changes to:
# - MrBAYES/mrbayes_3.2.1/Makefile
# - PROBCONS/probcons/Makefile
# - FASTTREE/Makefile

# --------------------------------------------------------------------------------

$(MAIN):
ifeq ('$(RAISE_WARNINGS)','0')
	@echo ----------------- non-ARB code: warnings disabled
endif
	$(MAKE) $(ARCHS)
ifeq ('$(RAISE_WARNINGS)','0')
	@echo ----------------- warnings enabled again
endif

depends: $(ARCHS_DEPENDS:.dummy=.depend)

clean: $(ARCHS:.dummy=.clean)

%.depend:
	@cp -p $(@D)/Makefile $(@D)/Makefile.old # save old Makefile
	@$(MAKE) -C $(@D) -r depends
	@grep "^# DO NOT DELETE" $(@D)/Makefile >/dev/null || \
		(echo "$(@D)/Makefile:666: Error: No dependencies in $(@D)/Makefile"; false) # check whether sub Makefile has dependencies
	@$(ARBHOME)/SOURCE_TOOLS/fix_depends.pl "(from GDE)" < $(@D)/Makefile > $(@D)/Makefile.2
	@mv $(@D)/Makefile.old $(@D)/Makefile # restore old Makefile
	@$(ARBHOME)/SOURCE_TOOLS/mv_if_diff $(@D)/Makefile.2 $(@D)/Makefile # update Makefile if changed

%.dummy:
	+@( \
	(( \
	    echo "$(SEP) Make $(@D)"; \
	    $(MAKE) -C $(@D) -r \
		"ARB  = yes" \
		"MAIN = $(@F:.dummy=.a)" \
		"cflags = $(sub_cflags) -DIN_ARB_$(@D:/=)" && \
	    echo "$(SEP) Make $(@D) [done]"; \
	) >$(@D).$$ID.log 2>&1 && (cat $(@D).$$ID.log;rm $(@D).$$ID.log)) || (cat $(@D).$$ID.log;rm $(@D).$$ID.log;false))

%.clean:
	+@$(MAKE) -C $(@D) \
		"ARB  = yes" \
		clean

%.proto:
	@$(MAKE) -C $(@D) \
		proto

# the end of the above command avoids that the output of parallel make calls gets mixed up 

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)
