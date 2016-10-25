
ARCHS_ALL = \
	glpng/libglpng_arb.dummy  \
	glAW/libglAW.dummy  \

# --------------------------------------------------------------------------------
# If a package is available as debian package it should go here

ARCHS_NON_DEBIAN = \

# --------------------------------------------------------------------------------

ifeq ($(DEBIAN),1)
ARCHS = $(ARCHS_ALL) 
else
ARCHS = $(ARCHS_ALL) $(ARCHS_NON_DEBIAN)
endif

# --------------------------------------------------------------------------------
# warnings in this subtree?

RAISE_WARNINGS=1

ifeq ($(RAISE_WARNINGS),0)
cflags:=$(cflags:-W -Wall=-w)
endif

include $(ARBHOME)/SOURCE_TOOLS/export2sub

# --------------------------------------------------------------------------------

$(MAIN): $(ARCHS)

depends: $(ARCHS:.dummy=.depend)

clean: $(ARCHS:.dummy=.clean)

%.depend:
	@cp -p $(@D)/Makefile $(@D)/Makefile.old # save old Makefile
	@$(MAKE) -C $(@D) -r depends
	@grep "^# DO NOT DELETE" $(@D)/Makefile >/dev/null # check whether sub Makefile has dependencies
	@cat $(@D)/Makefile \
		| ../SOURCE_TOOLS/fix_depends.pl "(from GL)" \
		>$(@D)/Makefile.2
	@mv $(@D)/Makefile.old $(@D)/Makefile # restore old Makefile
	@$(ARBHOME)/SOURCE_TOOLS/mv_if_diff $(@D)/Makefile.2 $(@D)/Makefile # update Makefile if changed

%.dummy:
	@$(MAKE) -C $(@D) -r \
		"ARB  = yes" \
		"MAIN = $(@F:.dummy=.a)" \
		"cflags = $(cflags) -DIN_ARB_$(@D:/=)" \

%.clean:
	@$(MAKE) -C $(@D) \
		clean

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)
