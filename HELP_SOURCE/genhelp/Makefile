
STD = standard.header
AUTOSECTION = sed -e 's/^\([^ ]\)/SECTION \1/'
AUTOTITLE = sed -e '1 s/^SECTION/TITLE/'
ARBHOME=../..

# --------------------------------------------------------------------------------

HLP = copyright.hlp changes.hlp

# --------------------------------------------------------------------------------

all : $(HLP)

$(HLP) : $(STD) Makefile

%.hlp : %.header

copyright.hlp : $(ARBHOME)/arb_LICENSE.txt
		cp $(STD) $@
		cat $< | $(AUTOSECTION) | $(AUTOTITLE) >> $@

changes.hlp : $(ARBHOME)/arb_CHANGES.txt
		cp $(STD) $@
		cat $< | $(AUTOSECTION) | $(AUTOTITLE) >> $@

clean:
		-rm $(HLP)


