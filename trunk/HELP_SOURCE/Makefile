
.SUFFIXES: .o .c .cxx .source .hlp .xml .html

# --------------------------------------------------------------------------------
# directories of source/destination files:

HLP_SOURCE=oldhelp
HLP_GENERATED=genhelp
XML_LOCATION=Xml
HTML_LOCATION=../lib/help_html
HLP_DEST=../lib/help

# choose conversion mode:
CONVERT_FROM_OLD_HELP=1# WARNING! If set to 1 => overwrites XML-Files (that's intended for now!)
# if set to 0 the helpfiles in oldhelp are ignored and only XML is used as source (not intended for now)

# needed tools:
TOOL_OBJECT = \
	 arb_help2xml.o

# I use Sablotron 0.71
XSLT = sabcmd
VALIDATE = rxp -v -V -V

# --------------------------------------------------------------------------------

all: date generated dirs tools delzerohtml
ifeq ($(CONVERT_FROM_OLD_HELP),1)
		@echo ---------------------------------------------------
		@echo ------------ Converting old hlp 2 xml:
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
# Generate help files in $(HLP_GENERATED):

generated:
		@echo ---------------------------------------------------
		@echo ------------ Generating some help files:
		(cd $(HLP_GENERATED);$(MAKE) all)

# --------------------------------------------------------------------------------

ifeq ($(CONVERT_FROM_OLD_HELP),1)

#OLD_HELP= oldhelp/importift.hlp
OLD_HELP_ALL= \
		$(HLP_SOURCE)/arb.hlp \
		$(wildcard \
				$(HLP_GENERATED)/*.hlp \
				$(HLP_SOURCE)/*.hlp \
				$(HLP_SOURCE)/seer/*.hlp \
		)

OLD_HELP=$(OLD_HELP_ALL:$(HLP_SOURCE)/FORM.hlp=)# remove FORM.hlp (this is just a default file)
XML_TMP=$(OLD_HELP:%.hlp=%.xml)
XML_TMP2=$(XML_TMP:$(HLP_SOURCE)/%=$(XML_LOCATION)/%)
XML=$(XML_TMP2:$(HLP_GENERATED)/%=$(XML_LOCATION)/%)

$(XML) : $(HELP2XML)

xml : $(XML)

dump:
		echo $(XML)

$(XML_LOCATION)/%.xml : $(HLP_SOURCE)/%.hlp
		@test \! -f $(HLP_GENERATED)/$(<F) || \
				( echo $<:1: exists twice -- only one existance allowed; \
				  echo $(HLP_GENERATED)/$(<F):1: other occurance \
				  && false )
		arb_help2xml $< $@
		$(VALIDATE) $@ >/dev/null

$(XML_LOCATION)/%.xml : $(HLP_GENERATED)/%.hlp
		@test \! -f $(HLP_SOURCE)/$(<F) || \
				( echo $<:1: exists twice -- only one existance allowed; \
				  echo $(HLP_SOURCE)/$(<F):1: other occurance \
				  && false )
		arb_help2xml $< $@
		$(VALIDATE) $@ >/dev/null

else

XML=$(wildcard $(XML_LOCATION)/*.xml $(XML_LOCATION)/seer/*.xml)

endif

HTML_TMP=$(XML:%.xml=%.html)
HTML=$(HTML_TMP:$(XML_LOCATION)/%=$(HTML_LOCATION)/%)

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
		$(XSLT) $(STYLE_HELP) $< $@ '$$myname=$(subst $(XML_LOCATION)/,,$<)' '$$xml_location=$(XML_LOCATION)' || rm $@

# --------------------------------------------------------------------------------

$(HTML) : $(STYLE_HTML) $(DTD)

# Google logo
google_logo: $(HTML_LOCATION)/Logo_25wht.gif
$(HTML_LOCATION)/Logo_25wht.gif : Logo_25wht.gif
		cp -p $< $@

$(HTML_LOCATION)/%.html : $(XML_LOCATION)/%.xml
		@-test -f $@ && rm $@
		$(XSLT) $(STYLE_HTML) $< $@ '$$myname=$(subst $(XML_LOCATION)/,,$<)' '$$xml_location=$(XML_LOCATION)' || (rm $@ && test -f $@)

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

html : $(HTML) google_logo

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
		(cd $(HLP_GENERATED);$(MAKE) clean)

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
