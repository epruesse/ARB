
.SUFFIXES: .o .c .cxx

# --------------------------------------------------------------------------------
# directories of source/destination files:

HLP_SOURCE=oldhelp
XML_LOCATION=Xml
HTML_LOCATION=../lib/help_html
HLP_DEST=../lib/help

# choose conversion mode:
CONVERT_FROM_OLD_HELP=1# WARNING! If set to 1 => overwrites XML-Files (that's ok for now!)

# needed tools:
TOOL_OBJECT = \
	 arb_help2xml.o

# I use Sablotron 0.71
XSLT = sabcmd
VALIDATE = rxp -v -V -V

# --------------------------------------------------------------------------------

all: date dirs tools delzerohtml
ifeq ($(CONVERT_FROM_OLD_HELP),1)
		@echo ---------------------------------------------------
		@echo ------------ Converting old hlp  2 xml:
		$(MAKE) xml
endif
		@echo ---------------------------------------------------
		@echo ------------ Converting xml 2 hlp:
		$(MAKE) help
		@echo ---------------------------------------------------
		@echo ------------ Converting xml 2 html:
		$(MAKE) html
		$(MAKE) html_index

dirs:
		(test -d $(XML_LOCATION)) || mkdir $(XML_LOCATION)
		(test -d $(XML_LOCATION)/seer) || mkdir $(XML_LOCATION)/seer
		(test -d $(HTML_LOCATION)) || mkdir $(HTML_LOCATION)
		(test -d $(HTML_LOCATION)/seer) || mkdir $(HTML_LOCATION)/seer
		(test -d $(HLP_DEST)) || mkdir $(HLP_DEST)
		(test -d $(HLP_DEST)/seer) || mkdir $(HLP_DEST)/seer
		@-ln -s ../arb_help.dtd $(XML_LOCATION)
		@-ln -s ../../arb_help.dtd $(XML_LOCATION)/seer

HELP2XML=$(ARBHOME)/bin/arb_help2xml

# --------------------------------------------------------------------------------

ifeq ($(CONVERT_FROM_OLD_HELP),1)

#OLD_HELP= oldhelp/importift.hlp
OLD_HELP= $(HLP_SOURCE)/arb.hlp $(wildcard $(HLP_SOURCE)/*.hlp $(HLP_SOURCE)/seer/*.hlp)
XML_TMP=$(OLD_HELP:%.hlp=%.xml)
XML=$(XML_TMP:$(HLP_SOURCE)/%=$(XML_LOCATION)/%)

$(XML) : $(HELP2XML)

xml : $(XML)

$(XML_LOCATION)/%.xml : $(HLP_SOURCE)/%.hlp
		arb_help2xml $< $@
		$(VALIDATE) $@ >/dev/null

else

XML=$(wildcard $(XML_LOCATION)/*.xml $(XML_LOCATION)/seer/*.xml)

endif

HTML_TMP=$(XML:%.xml=%.html)
HTML=$(HTML_TMP:$(XML_LOCATION)/%=$(HTML_LOCATION)/%)
#HTML=html/acisrt.html html/seer/login.html

HELP_TMP=$(XML:%.xml=%.hlp)
HELP=$(HELP_TMP:$(XML_LOCATION)/%=$(HLP_DEST)/%)

# --------------------------------------------------------------------------------

delzerohtml:
		-find $(HTML_LOCATION) -name "*.html" -size -1 -exec rm {} \;

date:
		$(MAKE) date.xsl

date.xsl :
		cat date.xsl.head >$@
		date '+%d.%b %Y' >>$@
		cat date.xsl.foot >>$@

# --------------------------------------------------------------------------------

DTD=arb_help.dtd
STYLE_HTML=to_html.xsl
STYLE_HELP=to_help.xsl

# --------------------------------------------------------------------------------

$(HELP) : $(STYLE_HELP) $(DTD)

$(HLP_DEST)/%.hlp : $(XML_LOCATION)/%.xml
		@-test -f $@ && rm $@
		$(XSLT) $(STYLE_HELP) $< $@ '$$myname=$(subst $(XML_LOCATION)/,,$<)' '$$xml_location=$(XML_LOCATION)' \
				|| rm $@

# --------------------------------------------------------------------------------

$(HTML) : $(STYLE_HTML) $(DTD)

$(HTML_LOCATION)/%.html : $(XML_LOCATION)/%.xml
		@-test -f $@ && rm $@
		$(XSLT) $(STYLE_HTML) $< $@ '$$myname=$(subst $(XML_LOCATION)/,,$<)' '$$xml_location=$(XML_LOCATION)' \
				|| (rm $@ && test -f $@)

# --------------------------------------------------------------------------------

HTML_IDX = _index.html

html.list: $(HTML)
		find $(HTML_LOCATION) -name "*.html" >$@

$(HTML_IDX) : html.list
		echo '<HTML><HEAD><TITLE>Arb-Help Index</TITLE></HEAD>' >$@
		echo '<BODY>' >>$@
		cat $< |  sed -e 's/^\(.*\)$$/<A href="\1">\1<\/A> /'  >>$@
		echo '</BODY></HTML>' >>$@

# --------------------------------------------------------------------------------

html : $(HTML)

html_index : $(HTML_IDX)

# --------------------------------------------------------------------------------

help : $(HELP)

# --------------------------------------------------------------------------------

clean:
		-rm $(HTML_IDX) `find $(HLP_DEST) -name "*.hlp"`
		-rm html.list `find $(HTML_LOCATION) -name "*.html"`
ifeq ($(CONVERT_FROM_OLD_HELP),1)
		-rm dummy `find $(XML_LOCATION) -name "*.xml"`
endif

# --------------------------------------------------------------------------------

tools: $(TOOL_OBJECT:%.o=$(ARBHOME)/bin/%)

LIBS=../XML/XML.a

$(HELP2XML): $(LIBS)

$(HELP2XML): arb_help2xml.cxx
	$(CPP) $(cflags) -o $@ $< $(CPPINCLUDES) $(LIBS)

$(ARBHOME)/bin/%: %.c
	$(ACC) $(cflags) -o $@ $< $(AINCLUDES)

$(ARBHOME)/bin/%: %.cxx
	$(CPP) $(cflags) -o $@ $< $(CPPINCLUDES) $(LIB2)

depend:
	$(MAKEDEPEND) -o '' -p $(ARBHOME)/bin/ $(MAKEDEPENDINC) $(OBJECT:.o=.c*)

# DO NOT DELETE
