
.SUFFIXES: .o .c .cxx .source .hlp .xml .html .depend .ps.gz .pdf.gz

# --------------------------------------------------------------------------------

TEST_ONE=0# default is 0 (if 1 only arb.hlp is processed)

# --------------------------------------------------------------------------------
# directories of source/destination files:

HLP_SOURCE=oldhelp
HLP_GENERATED=genhelp
XML_LOCATION=Xml
HTML_LOCATION=../lib/help_html
HLP_DEST=../lib/help

# choose conversion mode:
CONVERT_FROM_OLD_HELP=1# WARNING! If set to 1 => overwrites XML-Files (that IS intended for now!)
# if set to 0 the helpfiles in oldhelp are ignored and only XML is used as source (not intended for now)

# needed tools:
TOOL_OBJECT = \
	 arb_help2xml.o

QUIETLY = ./quietly.pl

# We use Sablotron 1.0 and RXP 1.2
XSLT = sabcmd
VALIDATE = $(QUIETLY) RXP
#VALIDATE = ./quiet_rxp.sh
#VALIDATE = rxp -v -V -V

# --------------------------------------------------------------------------------

all: date generated dirs tools nonhlp_formats delzerohtml
ifeq ($(CONVERT_FROM_OLD_HELP),1)
		@echo ---------------------------------------------------
		@echo ------------ Converting old hlp 2 xml:
		$(MAKE) xml
endif
		@echo ---------------------------------------------------
		@echo ------------ Converting xml 2 hlp:
		$(MAKE) help
		$(MAKE) $(HLP_DEST)/FORM.hlp
		@echo ---------------------------------------------------
		@echo ------------ Converting xml 2 html:
		$(MAKE) html
		$(MAKE) html_index

$(HLP_DEST)/FORM.hlp: $(HLP_SOURCE)/FORM.hlp
		cp -p $< $@


dirs:
		mkdir -p $(XML_LOCATION)
		mkdir -p $(XML_LOCATION)/seer
		mkdir -p $(XML_LOCATION)/prompt

		mkdir -p $(HTML_LOCATION)
		mkdir -p $(HTML_LOCATION)/seer
		mkdir -p $(HTML_LOCATION)/prompt

		mkdir -p $(HLP_DEST)
		mkdir -p $(HLP_DEST)/seer
		mkdir -p $(HLP_DEST)/prompt

		@-ln -s ../arb_help.dtd $(XML_LOCATION)
		@-ln -s ../../arb_help.dtd $(XML_LOCATION)/seer
		@-ln -s ../../arb_help.dtd $(XML_LOCATION)/prompt

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
				$(HLP_SOURCE)/prompt/*.hlp \
		)

ifeq ($(TEST_ONE),1)
#OLD_HELP=$(HLP_SOURCE)/parser.hlp $(HLP_SOURCE)/commands.hlp $(HLP_SOURCE)/FAQS.hlp $(HLP_SOURCE)/acisrt.hlp
#OLD_HELP=$(HLP_SOURCE)/arb.hlp
OLD_HELP=$(HLP_SOURCE)/help.hlp
else
OLD_HELP=$(OLD_HELP_ALL:$(HLP_SOURCE)/FORM.hlp=)# remove FORM.hlp (this is just a default file)
endif

XML_TMP=$(OLD_HELP:%.hlp=%.xml)
XML_TMP2=$(XML_TMP:$(HLP_SOURCE)/%=$(XML_LOCATION)/%)
XML=$(XML_TMP2:$(HLP_GENERATED)/%=$(XML_LOCATION)/%)

XML_DEPEND=xml.stamp# xml files are rebuild if this stamp is recreated

$(XML) : $(XML_DEPEND)  $(QUIETLY)

#$(XML) : $(XML_DEPEND) Makefile $(QUIETLY)

xml : $(XML)

dump:
		echo $(XML)

$(XML_LOCATION)/%.xml : $(HLP_SOURCE)/%.hlp
		@test \! -f $(HLP_GENERATED)/$(<F) || \
				( echo $<:1: exists twice -- only one existance allowed; \
				  echo $(HLP_GENERATED)/$(<F):1: other occurance \
				  && false )
		arb_help2xml $< $@
		$(VALIDATE) $@

$(XML_LOCATION)/%.xml : $(HLP_GENERATED)/%.hlp
		@test \! -f $(HLP_SOURCE)/$(<F) || \
				( echo $<:1: exists twice -- only one existance allowed; \
				  echo $(HLP_SOURCE)/$(<F):1: other occurance \
				  && false )
		arb_help2xml $< $@
		$(VALIDATE) $@

else

XML=$(wildcard $(XML_LOCATION)/*.xml $(XML_LOCATION)/seer/*.xml) $(XML_LOCATION)/prompt/*.xml)

endif

HTML_TMP=$(XML:%.xml=%.html)
HTML=$(HTML_TMP:$(XML_LOCATION)/%=$(HTML_LOCATION)/%)

HELP_TMP=$(XML:%.xml=%.hlp)
HELP=$(HELP_TMP:$(XML_LOCATION)/%=$(HLP_DEST)/%)

# --------------------------------------------------------------------------------

PS_SRC=$(wildcard $(HLP_SOURCE)/*.ps.gz)
PDF_SRC=$(wildcard $(HLP_SOURCE)/*.pdf.gz)

PS_DEST=$(PS_SRC:$(HLP_SOURCE)/%=$(HLP_DEST)/%)
PDF_DEST=$(PDF_SRC:$(HLP_SOURCE)/%=$(HLP_DEST)/%)

ps: $(PS_DEST)

pdf: $(PDF_DEST)

nonhlp_formats: ps pdf

$(HLP_DEST)/%.ps.gz : $(HLP_SOURCE)/%.ps.gz
	cp -p $< $@

$(HLP_DEST)/%.pdf.gz : $(HLP_SOURCE)/%.pdf.gz
	cp -p $< $@

# --------------------------------------------------------------------------------

delzerohtml:
		-find $(HTML_LOCATION) -name "*.html" -size -1 -exec rm {} \;

date :
	$(MAKE) date.xsl.tmp
	../SOURCE_TOOLS/mv_if_diff date.xsl.tmp date.xsl

date.xsl.tmp :
		cat date.xsl.header >$@
		bash -c "(export LC_ALL=C;date '+%d. %b %Y')" >>$@
		cat date.xsl.footer >>$@

date.xsl : date

# --------------------------------------------------------------------------------

DTD=arb_help.dtd
STYLE_HTML=to_html.xsl
STYLE_HELP=to_help.xsl

# --------------------------------------------------------------------------------

$(HELP) : $(STYLE_HELP) $(DTD)

$(HLP_DEST)/%.hlp : $(XML_LOCATION)/%.xml
		@-test -f $@ && rm $@
		$(XSLT) $(STYLE_HELP) $< $@ '$$myname=$(subst $(XML_LOCATION)/,,$<)' '$$xml_location=$(XML_LOCATION)' || rm $@
		@test -f $@

# --------------------------------------------------------------------------------

$(HTML) : $(STYLE_HTML) $(DTD)

# Google logo
google_logo: $(HTML_LOCATION)/Logo_25wht.gif
$(HTML_LOCATION)/Logo_25wht.gif : Logo_25wht.gif
		cp -p $< $@
		@test -f $@

$(HTML_LOCATION)/%.html : $(XML_LOCATION)/%.xml
		@-test -f $@ && rm $@
		$(XSLT) $(STYLE_HTML) $< $@ '$$myname=$(subst $(XML_LOCATION)/,,$<)' '$$xml_location=$(XML_LOCATION)' || (rm $@ && test -f $@)
		@test -f $@

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
		-rm $(XML_DEPEND) dummy `find $(XML_LOCATION) -name "*.xml"`
endif
		(cd $(HLP_GENERATED);$(MAKE) clean)

# --------------------------------------------------------------------------------

tools: $(TOOL_OBJECT:%.o=$(ARBHOME)/bin/%)

LIBS=../XML/XML.a

$(HELP2XML): $(LIBS)

$(XML_DEPEND) : arb_help2xml.cxx
	touch $(XML_DEPEND)

$(HELP2XML): arb_help2xml.cxx
	$(CPP) $(cflags) -o $@ $< $(CPPINCLUDES) $(LIBS)

$(ARBHOME)/bin/%: %.c
	$(ACC) $(cflags) -o $@ $< $(AINCLUDES)

$(ARBHOME)/bin/%: %.cxx
	$(CPP) $(cflags) -o $@ $< $(CPPINCLUDES) $(LIB2)

DEPENDS = $(TOOL_OBJECT:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

arb_help2xml.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/xml.hxx
