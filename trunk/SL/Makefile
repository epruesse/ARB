
ARCHS_ALL = \
	HELIX/HELIX.dummy  \
	AW_HELIX/AW_HELIX.dummy  \
	AW_NAME/AW_NAME.dummy  \
	DB_SCANNER/DB_SCANNER.dummy  \
	FILE_BUFFER/FILE_BUFFER.dummy  \
	REGEXPR/REGEXPR.dummy  \

# --------------------------------------------------------------------------------
# If a package is available as debian package it should go here

ARCHS_NON_DEBIAN = \

# --------------------------------------------------------------------------------

ifdef DEBIAN
ARCHS = $(ARCHS_ALL) 
else
ARCHS = $(ARCHS_ALL) $(ARCHS_NON_DEBIAN)
endif

# --------------------------------------------------------------------------------
# warnings in this subtree?

RAISE_WARNINGS=1

ifeq ($(RAISE_WARNINGS),0)
CPP:=$(CPP:-W -Wall=-w)
ACC:=$(ACC:-W -Wall=-w)
endif

include $(ARBHOME)/SOURCE_TOOLS/export2sub

# --------------------------------------------------------------------------------

$(MAIN): $(ARCHS)

depends: $(ARCHS:.dummy=.depend)

clean: $(ARCHS:.dummy=.clean)

%.depend:
	@cp -p $(@D)/Makefile $(@D)/Makefile.old # save old Makefile
	@$(MAKE) -C $(@D) -r  depends
	@grep "^# DO NOT DELETE" $(@D)/Makefile >/dev/null # check whether sub Makefile has dependencies
	@cat $(@D)/Makefile \
		| ../SOURCE_TOOLS/fix_depends.pl \
		>$(@D)/Makefile.2
	@mv $(@D)/Makefile.old $(@D)/Makefile # restore old Makefile
	@$(ARBHOME)/SOURCE_TOOLS/mv_if_diff $(@D)/Makefile.2 $(@D)/Makefile # update Makefile if changed

%.dummy:
	@(($(MAKE) -C $(@D) -r \
		"ARB  = yes" \
		"MAIN = $(@F:.dummy=.a)" \
		"cflags = $(cflags) -DIN_ARB_$(@D:/=)" \
		>$(@D).log 2>&1 && (cat $(@D).log;rm $(@D).log)) || (cat $(@D).log;rm $(@D).log;false))

%.clean:
	@$(MAKE) -C $(@D) \
		clean

# the end of the above command avoids that the output of parallel make calls gets mixed up 

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl
