
.SUFFIXES: .o .c .cxx

# --------------------------------------------------------------------------------
# directories of source/destination files:

HLP_SOURCE=oldhelp
XML_LOCATION=Xml
HTML_LOCATION=Html
HLP_DEST=Newhelp

# choose conversion mode:
CONVERT_FROM_OLD_HELP=1# WARNING! If set to 1 => overwrites XML-Files (backup first)

# needed tools:
TOOL_OBJECT = \
	 arb_help2xml.o

# --------------------------------------------------------------------------------

all: date dirs tools delzerohtml
ifeq ($(CONVERT_FROM_OLD_HELP),1)
		$(MAKE) xml
endif
		$(MAKE) html
		$(MAKE) html_index
#		$(MAKE) help

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

#OLD_HELP= oldhelp/probematch.hlp
OLD_HELP= $(wildcard $(HLP_SOURCE)/*.hlp $(HLP_SOURCE)/seer/*.hlp)
XML_TMP=$(OLD_HELP:%.hlp=%.xml)
XML=$(XML_TMP:$(HLP_SOURCE)/%=$(XML_LOCATION)/%)

$(XML) : $(HELP2XML)

xml : $(XML)

$(XML_LOCATION)/%.xml : $(HLP_SOURCE)/%.hlp
		arb_help2xml $< $@

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

clean:
		-rm $(HTML)
ifeq ($(CONVERT_FROM_OLD_HELP),1)
		-rm $(XML)
endif

date:
		$(MAKE) date.xsl

date.xsl :
		cat date.xsl.head >$@
		date '+%d.%b %Y' >>$@
		cat date.xsl.foot >>$@

# --------------------------------------------------------------------------------

XSLT = sabcmd
VALIDATE = rxp -v -V -V

DTD=arb_help.dtd
STYLE_HTML=to_html.xsl
STYLE_HELP=to_help.xsl

$(HTML) : $(STYLE_HTML) $(DTD)

$(HTML_LOCATION)/%.html : $(XML_LOCATION)/%.xml
		@-test -f $@ && rm $@
		$(VALIDATE) $< >/dev/null
		$(XSLT) $(STYLE_HTML) $< $@ '$$myname=$(subst $(XML_LOCATION)/,,$<)' '$$xml_location=$(XML_LOCATION)'

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
