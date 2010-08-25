
STD = standard.header
AUTOSECTION = sed -e 's/^\([^ \#]\)/SECTION \1/'
AUTOTITLE = sed -e '1 s/^SECTION/TITLE/'
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

copyright.hlp : $(ARBHOME)/arb_LICENSE.txt
	@echo Updating $@ from $< 
	@cp $(STD) $@
	@echo "# BUILD FROM: $<" >>$@
	@cat $< | $(AUTOSECTION) | $(AUTOTITLE) >> $@

changes.hlp : $(ARBHOME)/arb_CHANGES.txt
	@echo Updating $@ from $< 
	@cp $(STD) $@
	@echo "# BUILD FROM: $<" >>$@
	@cat $< | $(AUTOSECTION) | $(AUTOTITLE) >> $@

agde_%.hlp : $(GDEGENDIR)/%.help
	@echo Updating $@ from $< 
	@cp $(STD) $@
	@echo "# BUILD FROM: $<" >>$@
	@echo "#             (which is generated itself)" >>$@
	@echo "" >>$@
	@echo "UP agde.hlp" >>$@
	@cat $< | $(AUTOSECTION) | $(AUTOTITLE) >> $@

agde_%.hlp : $(GDEWRITTENDIR)/%.help
	@echo Updating $@ from $< 
	@cp $(STD) $@
	@echo "# BUILD FROM: $<" >>$@
	@echo "" >>$@
	@echo "UP agde.hlp" >>$@
	@cat $< | $(AUTOSECTION) | $(AUTOTITLE) >> $@

agde.hlp: $(GDE_HLP) agde.footer
	@echo Updating GDE overview $@ 
	@cp $(STD) $@
	@echo "# BUILD FROM: $<" >>$@
	@ls agde_*.hlp | grep -v 'agde_.*_sub'| sed -e 's/^/SUB /' >>$@
	@echo "" >>$@
	@echo "UP gde_menus.hlp" >>$@
	@cat agde.footer >>$@

clean:
		rm -f *.hlp


