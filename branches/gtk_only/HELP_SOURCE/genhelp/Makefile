
.PHONY: clean all

STD = standard.header
SED:=$(ARBHOME)/SH/arb_sed

POSTPROCESS=./postprocess_genhelp.pl
HLP_DEPENDS=Makefile $(POSTPROCESS)

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

copyright.hlp : $(ARBHOME)/arb_LICENSE.txt $(HLP_DEPENDS)
	@echo Updating $@ from $< 
	@(test -f $@ && chmod u+w $@) || true
	@cp $(STD) $@
	@echo "# BUILD FROM: $<" >>$@
	@cat $< | $(POSTPROCESS) $@ >> $@
	@chmod a-w $@

changes.hlp : $(ARBHOME)/arb_CHANGES.txt $(HLP_DEPENDS)
	@echo Updating $@ from $< 
	@(test -f $@ && chmod u+w $@) || true
	@cp $(STD) $@
	@echo "# BUILD FROM: $<" >>$@
	@cat $< | $(POSTPROCESS) $@ >> $@
	@@chmod a-w $@

agde_%.hlp : $(GDEGENDIR)/%.help $(HLP_DEPENDS)
	@echo Updating $@ from $< 
	@(test -f $@ && chmod u+w $@) || true
	@cp $(STD) $@
	@echo "# BUILD FROM: $<" >>$@
	@echo "#             (which is generated itself)" >>$@
	@echo "" >>$@
	@echo "UP agde.hlp" >>$@
	@cat $< | $(POSTPROCESS) $@ >> $@
	@chmod a-w $@

agde_%.hlp : $(GDEWRITTENDIR)/%.help $(HLP_DEPENDS)
	@echo Updating $@ from $< 
	@(test -f $@ && chmod u+w $@) || true
	@cp $(STD) $@
	@echo "# BUILD FROM: $<" >>$@
	@echo "" >>$@
	@echo "UP agde.hlp" >>$@
	@cat $< | $(POSTPROCESS) $@ >> $@
	@chmod a-w $@

agde.hlp: $(GDE_HLP) agde.footer Makefile
	@echo Updating GDE overview $@ 
	@(test -f $@ && chmod u+w $@) || true
	@cp $(STD) $@
	@echo "# BUILD FROM: $<" >>$@
	@ls agde_*.hlp | grep -v 'agde_.*_sub'| $(SED) -e 's/^/SUB /' >>$@
	@echo "" >>$@
	@echo "UP gde_menus.hlp" >>$@
	@cat agde.footer >>$@
	@chmod a-w $@

clean:
	rm -f *.hlp

