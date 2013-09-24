
.PHONY: clean all

STD = standard.header
SED:=$(ARBHOME)/SH/arb_sed

AUTOSECTION = $(SED) -e 's/^\([^ \#]\)/SECTION \1/'
AUTOTITLE = $(SED) -e '1 s/^SECTION/TITLE/'
ARBHOME=../..

GDEBASE=$(ARBHOME)/GDEHELP
GDEGENDIR=$(GDEBASE)/HELP_GEN
GDEWRITTENDIR=$(GDEBASE)/HELP_WRITTEN
GDE_HELP=$(wildcard $(GDEGENDIR)/*.help) $(wildcard $(GDEWRITTENDIR)/*.help)
GDE_HLP=$(addprefix agde_,$(subst .help,.hlp,$(notdir $(GDE_HELP))))

# --------------------------------------------------------------------------------

HLP = copyright.hlp changes.hlp agde.hlp

# --------------------------------------------------------------------------------

all : $(HLP) $(GDE_HLP)

$(HLP) : $(STD) Makefile
$(GDE_HLP) : $(STD) Makefile

%.hlp : %.header

copyright.hlp : $(ARBHOME)/arb_LICENSE.txt Makefile
	@echo Updating $@ from $< 
	@-test -f $@ && chmod u+w $@
	@cp $(STD) $@
	@echo "# BUILD FROM: $<" >>$@
	@cat $< | $(AUTOSECTION) | $(AUTOTITLE) >> $@
	@chmod a-w $@

changes.hlp : $(ARBHOME)/arb_CHANGES.txt Makefile
	@echo Updating $@ from $< 
	@-test -f $@ && chmod u+w $@
	@cp $(STD) $@
	@echo "# BUILD FROM: $<" >>$@
	@cat $< | $(AUTOSECTION) | $(AUTOTITLE) >> $@
	@@chmod a-w $@

agde_%.hlp : $(GDEGENDIR)/%.help Makefile
	@echo Updating $@ from $< 
	@-test -f $@ && chmod u+w $@
	@cp $(STD) $@
	@echo "# BUILD FROM: $<" >>$@
	@echo "#             (which is generated itself)" >>$@
	@echo "" >>$@
	@echo "UP agde.hlp" >>$@
	@cat $< | $(AUTOSECTION) | $(AUTOTITLE) >> $@
	@chmod a-w $@

agde_%.hlp : $(GDEWRITTENDIR)/%.help Makefile
	@echo Updating $@ from $< 
	@-test -f $@ && chmod u+w $@
	@cp $(STD) $@
	@echo "# BUILD FROM: $<" >>$@
	@echo "" >>$@
	@echo "UP agde.hlp" >>$@
	@cat $< | $(AUTOSECTION) | $(AUTOTITLE) >> $@
	@chmod a-w $@

agde.hlp: $(GDE_HLP) agde.footer Makefile
	@echo Updating GDE overview $@ 
	@-test -f $@ && chmod u+w $@
	@cp $(STD) $@
	@echo "# BUILD FROM: $<" >>$@
	@ls agde_*.hlp | grep -v 'agde_.*_sub'| $(SED) -e 's/^/SUB /' >>$@
	@echo "" >>$@
	@echo "UP gde_menus.hlp" >>$@
	@cat agde.footer >>$@
	@chmod a-w $@

clean:
	rm -f *.hlp

