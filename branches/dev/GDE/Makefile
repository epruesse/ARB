
ARCHS_ALL = \
	CLUSTAL/CLUSTAL.dummy  \
	SUPPORT/SUPPORT.dummy \
	MOLPHY/MOLPHY.dummy \
	AxML/AxML.dummy \
	RAxML/RAxML.dummy \
	PHYML/PHYML.dummy \

# --------------------------------------------------------------------------------
# If a package is available as debian package it should go here
# 
# Note: I'm not sure whether this is a good idea. Due to limited manpower it often takes
# longer until ARB works with newer (incompatible) versions of these tools. --ralf

ARCHS_NON_DEBIAN = \
	CLUSTALW/CLUSTALW.dummy  \
	PHYLIP/PHYLIP.dummy \
	TREEPUZZLE/TREEPUZZLE.dummy \
	FASTDNAML/FASTDNAML.dummy \

# --------------------------------------------------------------------------------

ifdef DEBIAN
ARCHS = $(ARCHS_ALL)
else
ARCHS = $(ARCHS_ALL) $(ARCHS_NON_DEBIAN)
endif

# --------------------------------------------------------------------------------
# modify cflags for submakefiles 

RAISE_WARNINGS=0# no warnings in this subtree
UNIT_TESTS=0# no warnings in this subtree

sub_cflags:=$(cflags)

ifeq ('$(RAISE_WARNINGS)','0')
sub_cflags:=$(subst -W -Wall,-w,$(sub_cflags))
endif
ifeq ('$(UNIT_TESTS)','0')
sub_cflags:=$(subst -DUNIT_TESTS,,$(sub_cflags))
endif

# --------------------------------------------------------------------------------

$(MAIN):
ifeq ('$(RAISE_WARNINGS)','0')
	@echo ----------------- non-ARB code: warnings disabled
endif
	$(MAKE) $(ARCHS)
ifeq ('$(RAISE_WARNINGS)','0')
	@echo ----------------- warnings enabled again
endif

proto: MOLPHY/MOLPHY.proto

depends: $(ARCHS:.dummy=.depend)

clean: $(ARCHS:.dummy=.clean)

%.depend:
	@cp -p $(@D)/Makefile $(@D)/Makefile.old # save old Makefile
	@$(MAKE) -C $(@D) -r depends
	@grep "^# DO NOT DELETE" $(@D)/Makefile >/dev/null # check whether sub Makefile has dependencies
	@cat $(@D)/Makefile \
		| ../SOURCE_TOOLS/fix_depends.pl \
		>$(@D)/Makefile.2
	@mv $(@D)/Makefile.old $(@D)/Makefile # restore old Makefile
	@$(ARBHOME)/SOURCE_TOOLS/mv_if_diff $(@D)/Makefile.2 $(@D)/Makefile # update Makefile if changed

%.dummy:
	@(( \
	    $(MAKE) -C $(@D) -r \
		"ARB  = yes" \
		"MAIN = $(@F:.dummy=.a)" \
		"cflags = $(sub_cflags) -DIN_ARB_$(@D:/=)" \
		>$(@D).$$ID.log 2>&1 && (cat $(@D).$$ID.log;rm $(@D).$$ID.log)) || (cat $(@D).$$ID.log;rm $(@D).$$ID.log;false))

%.clean:
	@$(MAKE) -C $(@D) \
		"ARB  = yes" \
		clean

%.proto:
	@$(MAKE) -C $(@D) \
		proto

# the end of the above command avoids that the output of parallel make calls gets mixed up 

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl
