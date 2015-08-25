
ARCHS = \
	IMP_TREE/IMP_TREE.dummy \

# --------------------------------------------------------------------------------

#SAX_INCLUDES        = -I. -I${XERCESCROOT}/include 
#SAX_LIB_SEARCH_PATH = -L${XERCESCROOT}/lib
#SAX_LIBRARY_NAME    = -lxerces-c

#SUB_CFLAGS=$(cflags) $(SAX_INCLUDES) -DXML_USE_PTHREADS

include $(ARBHOME)/SOURCE_TOOLS/export2sub

# --------------------------------------------------------------------------------

$(MAIN):
ifeq ($(SHOWTODO),1)
	@echo "Makefile:20: Warning: XML_IMPORT is unfinished work"
endif
	true

#$(MAIN): $(ARCHS)

depends: $(ARCHS:.dummy=.depend)

%.depend:
	@cp -p $(@D)/Makefile $(@D)/Makefile.old # save old Makefile
	@$(MAKE) -C $(@D) -r depends
	@grep "^# DO NOT DELETE" $(@D)/Makefile >/dev/null # check whether sub Makefile has dependencies
	@cat $(@D)/Makefile \
		| ../SOURCE_TOOLS/fix_depends.pl "(from XML_IMPORT)" \
		>$(@D)/Makefile.2
	@mv $(@D)/Makefile.old $(@D)/Makefile # restore old Makefile
	@$(ARBHOME)/SOURCE_TOOLS/mv_if_diff $(@D)/Makefile.2 $(@D)/Makefile # update Makefile if changed

%.dummy:
	@$(MAKE) -C $(@D) -r \
		"ARB  = yes" \
		"MAIN = $(@F:.dummy=.a)" \
		"cflags = $(SUB_CFLAGS) -DIN_ARB_$(@D:/=)" \

# DO NOT DELETE
