
STD = standard.header
AUTOSECTION = sed -e 's/^\([^ ]\)/SECTION \1/'
AUTOTITLE = sed -e '1 s/^SECTION/TITLE/'
ARBHOME=../..

GDEDIR=$(ARBHOME)/GDEHELP/HELP_GEN
GDE_HELP=$(wildcard $(GDEDIR)/*.help)
GDE_HLP=$(addprefix agde_,$(subst .help,.hlp,$(notdir $(GDE_HELP))))

# --------------------------------------------------------------------------------

HLP = copyright.hlp changes.hlp agde.hlp

# --------------------------------------------------------------------------------

all : $(HLP) $(GDE_HLP)

$(HLP) : $(STD) Makefile
$(GDE_HLP) : $(STD) Makefile

%.hlp : %.header

copyright.hlp : $(ARBHOME)/arb_LICENSE.txt
	cp $(STD) $@
	echo "# BUILD FROM: $<" >>$@
	cat $< | $(AUTOSECTION) | $(AUTOTITLE) >> $@

changes.hlp : $(ARBHOME)/arb_CHANGES.txt
	cp $(STD) $@
	echo "# BUILD FROM: $<" >>$@
	cat $< | $(AUTOSECTION) | $(AUTOTITLE) >> $@

agde_%.hlp : $(GDEDIR)/%.help
	cp $(STD) $@
	echo "# BUILD FROM: $<" >>$@
	echo "UP agde.hlp" >>$@
	cat $< | $(AUTOSECTION) | $(AUTOTITLE) >> $@

agde.hlp: $(GDE_HLP) agde.footer
	cp $(STD) $@
	echo "# BUILD FROM: $<" >>$@
	ls agde_*.hlp | grep -v 'agde_.*_sub'| sed -e 's/^/SUB /' >>$@
	cat agde.footer >>$@

clean:
		-rm *.hlp


