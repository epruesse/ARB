
.SUFFIXES: .o .c .cxx .source .hlp .xml .dtd .html .depend .ps.gz .pdf.gz

# --------------------------------------------------------------------------------

TEST_ONE=0# default is 0 (if 1 only one .hlp file is processed. see below @ TEST_ONE)

# --------------------------------------------------------------------------------
# directories of source/destination files:

HLP_SOURCE=oldhelp
HLP_GENERATED=genhelp
XML_LOCATION=Xml
HTML_LOCATION=../lib/help_html
HLP_DEST=../lib/help
HELP_INDEX_NAME = aaa_help_index

# needed tools:
TOOL_OBJECTS = \
	 arb_help2xml.o

BINARIES = $(TOOL_OBJECTS:%.o=$(ARBHOME)/bin/%)

QUIETLY = ./quietly.pl

# using xsltproc and XMLLINT:
XSLTPROC = xsltproc
VALIDATE = $(QUIETLY) XMLLINT

# --------------------------------------------------------------------------------

all: date generated dirs tools nonhlp_formats delzerohtml
		@echo --------------------------------------------------- [old hlp 2 xml]
		time $(MAKE) xml
		@echo --------------------------------------------------- [xml 2 hlp]
		time $(MAKE) help
		$(MAKE) $(HLP_DEST)/FORM.hlp
		@echo --------------------------------------------------- [xml 2 html]
		time $(MAKE) html

$(HLP_DEST)/FORM.hlp: $(HLP_SOURCE)/FORM.hlp
		cp -p $< $@


dirs:
		mkdir -p $(XML_LOCATION)
		mkdir -p $(XML_LOCATION)/prompt

		mkdir -p $(HTML_LOCATION)
		mkdir -p $(HTML_LOCATION)/prompt

		mkdir -p $(HLP_DEST)
		mkdir -p $(HLP_DEST)/prompt

		@-ln -s ../arb_help.dtd $(XML_LOCATION)
		@-ln -s ../../arb_help.dtd $(XML_LOCATION)/prompt

HELP2XML=$(ARBHOME)/bin/arb_help2xml

# --------------------------------------------------------------------------------
# Generate help files in $(HLP_GENERATED):

generated:
		@echo ---------------------------------------------------
		@echo ------------ Generating some help files:
		(cd $(HLP_GENERATED);$(MAKE) all)

# --------------------------------------------------------------------------------

HELP_INDEX_XML = $(XML_LOCATION)/$(HELP_INDEX_NAME).xml

#OLD_HELP= oldhelp/importift.hlp
OLD_HELP_ALL= \
		$(HLP_SOURCE)/arb.hlp \
		$(wildcard \
				$(HLP_GENERATED)/*.hlp \
				$(HLP_SOURCE)/*.hlp \
				$(HLP_SOURCE)/prompt/*.hlp \
		)

ifeq ($(TEST_ONE),1)
#OLD_HELP=$(HLP_SOURCE)/parser.hlp $(HLP_SOURCE)/commands.hlp $(HLP_SOURCE)/FAQS.hlp $(HLP_SOURCE)/acisrt.hlp
#OLD_HELP=$(HLP_SOURCE)/arb.hlp $(HLP_SOURCE)/version.hlp $(HLP_SOURCE)/translate_dna_2_pro.hlp $(HLP_SOURCE)/species_join.hlp
OLD_HELP=$(HLP_SOURCE)/version.hlp
#OLD_HELP=$(HLP_SOURCE)/arb.hlp
else
OLD_HELP=$(OLD_HELP_ALL:$(HLP_SOURCE)/FORM.hlp=)# remove FORM.hlp (this is just a default file)
endif

XML_TMP=$(OLD_HELP:%.hlp=%.xml)
XML_TMP2=$(XML_TMP:$(HLP_SOURCE)/%=$(XML_LOCATION)/%)
XML=$(XML_TMP2:$(HLP_GENERATED)/%=$(XML_LOCATION)/%) 

XML_DEPEND=xml.stamp# xml files are rebuild if this stamp is recreated
DTD=arb_help.dtd

# @@@ reactivate again:
$(XML) : $(XML_DEPEND)  $(QUIETLY)
#$(XML) :  $(QUIETLY)

#$(XML) : $(XML_DEPEND) Makefile $(QUIETLY)


$(HELP_INDEX_XML): $(OLD_HELP_ALL) $(XML) ./generate_index.pl Makefile 
	./generate_index.pl $(HELP_INDEX_NAME).xml $(XML_LOCATION) > $(HELP_INDEX_XML)

xml : $(XML) $(HELP_INDEX_XML)

dump:
		echo $(XML)

$(XML_LOCATION)/%.xml : $(HLP_SOURCE)/%.hlp $(DTD)
		@test \! -f $(HLP_GENERATED)/$(<F) || \
				( echo $<:1: exists twice -- only one existence allowed; \
				  echo $(HLP_GENERATED)/$(<F):1: other occurrence \
				  && false )
		@$(HELP2XML) $< $@
		@$(VALIDATE) $@

$(XML_LOCATION)/%.xml : $(HLP_GENERATED)/%.hlp $(DTD)
		@test \! -f $(HLP_SOURCE)/$(<F) || \
				( echo $<:1: exists twice -- only one existence allowed; \
				  echo $(HLP_SOURCE)/$(<F):1: other occurrence \
				  && false )
		@$(HELP2XML) $< $@
		@$(VALIDATE) $@

$(HTML_LOCATION)/help_index.html : $(HELP_INDEX_XML)

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

STYLE_HTML=to_html.xsl
STYLE_HELP=to_help.xsl

# --------------------------------------------------------------------------------

XSLTPROC_ARGS=--nonet --nomkdir --stringparam xml_location "$(XML_LOCATION)"

$(HELP) : $(STYLE_HELP) $(DTD) Makefile

$(HLP_DEST)/%.hlp : $(XML_LOCATION)/%.xml 
		@-rm -f $@
		@$(XSLTPROC) --output $@ $(XSLTPROC_ARGS) --stringparam myname "$(subst $(XML_LOCATION)/,,$<)" $(STYLE_HELP) $< || rm $@
		@test -f $@ || (echo "$<:0: did not compile to $@" && false)

# --------------------------------------------------------------------------------

$(HTML) : $(STYLE_HTML) $(DTD) Makefile

# Google logo
google_logo: $(HTML_LOCATION)/Logo_25wht.gif
$(HTML_LOCATION)/Logo_25wht.gif : Logo_25wht.gif
		cp -p $< $@
		@test -f $@

$(HTML_LOCATION)/%.html : $(XML_LOCATION)/%.xml
		@-rm -f $@
		@$(XSLTPROC) --output $@ $(XSLTPROC_ARGS) --stringparam myname "$(subst $(XML_LOCATION)/,,$<)" $(STYLE_HTML) $< || (rm $@ && test -f $@)
		@test -f $@ || (echo "$<:0: did not compile to $@" && false)

# --------------------------------------------------------------------------------

html : $(HTML) google_logo $(HTML_LOCATION)/$(HELP_INDEX_NAME).html

help : $(HELP)

# --------------------------------------------------------------------------------

clean:
	rm -f $(TOOL_OBJECTS) $(BINARIES)
	rm -f `find $(HLP_DEST) -name "*.hlp"`
	rm -f `find $(HTML_LOCATION) -name "*.html"`
	rm -f $(XML_DEPEND) dummy `find $(XML_LOCATION) -name "*.xml"`
	$(MAKE) -C $(HLP_GENERATED) clean

# --------------------------------------------------------------------------------

tools: $(BINARIES)

LIBS=../XML/XML.a

$(HELP2XML): $(LIBS)

$(XML_DEPEND) : arb_help2xml.cxx
	touch $(XML_DEPEND)

%.o: %.cxx
	$(CPP) $(cflags) -c $< $(CPPINCLUDES) $(POST_COMPILE)

$(HELP2XML): arb_help2xml.o $(LIBS)
	$(LINK_EXECUTABLE) $@ $< $(LIBS) $(LIBPATH) $(ARBDB_LIB) $(EXECLIBS)

DEPENDS = $(TOOL_OBJECTS:.o=.depend)
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

arb_help2xml.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/attributes.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/test_global.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/test_unit.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/xml.hxx
