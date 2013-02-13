# --------------------------------------------------------------------------------
# when adding a new library (in a new SUBDIR), perform the following steps:
#
# - create new subdir and copy a recent Makefile into there		e.g. use ./REFENTRIES/Makefile
# - add a dependency vs 'links' or 'links_non_perl' 			in ../Makefile@ADD_links_non_perl
# - add it to the ARCHS_... definitions used for linking 		in ../Makefile@Individual_Programs_Section
# - add all externally visible headers					in ../SOURCE_TOOLS/generate_all_links.sh@ALIVIEW
#
# --------------------------------------------------------------------------------

# automatically handles all subdirectories
ARCHS:=$(shell find . -maxdepth 1 -type d)# find all direct sub-directories
ARCHS:=$(sort $(filter-out . .svn,$(subst ./,,$(ARCHS))))# remove prefix './' and get rid of '.' and '.svn'
ARCHS:=$(shell echo $(ARCHS) | perl -pn -e 's/([A-Za-z0-9_]+)/\1\/\1/g;')# replace not possible $(patsubst %,%/%,$(ARCHS))
ARCHS:=$(ARCHS:%=%.dummy)

# --------------------------------------------------------------------------------
# warnings in this subtree?

RAISE_WARNINGS=1

ifeq ($(RAISE_WARNINGS),0)
CPP:=$(CPP:-W -Wall=-w)
ACC:=$(ACC:-W -Wall=-w)
endif

include $(ARBHOME)/SOURCE_TOOLS/export2sub

# --------------------------------------------------------------------------------

nosuchtarget:
	false

depends: $(ARCHS:.dummy=.depend)

clean: $(ARCHS:.dummy=.clean)

proto: TREEDISP/TREEDISP.proto 

%.depend:
	@cp -p $(@D)/Makefile $(@D)/Makefile.old # save old Makefile
	@$(MAKE) -C $(@D) -r  depends
	@grep "^# DO NOT DELETE" $(@D)/Makefile >/dev/null # check whether sub Makefile has dependencies
	@cat $(@D)/Makefile \
		| ../SOURCE_TOOLS/fix_depends.pl "(from SL)" \
		>$(@D)/Makefile.2
	@mv $(@D)/Makefile.old $(@D)/Makefile # restore old Makefile
	@$(ARBHOME)/SOURCE_TOOLS/mv_if_diff $(@D)/Makefile.2 $(@D)/Makefile # update Makefile if changed

%.dummy:
	@(($(MAKE) -C $(@D) -r \
		"ARB  = yes" \
		"MAIN = $(@F:.dummy=.a)" \
		"cflags = $(cflags) -DIN_ARB_$(@D:/=)" \
		>$(@D).log 2>&1 && (cat $(@D).log;rm $(@D).log)) || (cat $(@D).log;rm $(@D).log;false))

# the logging through files is done to avoid that the output of parallel calls gets mixed up (make -j..)

%.clean:
	@$(MAKE) -C $(@D) \
		clean

%.proto:
	@$(MAKE) -C $(@D) \
		proto


# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)
