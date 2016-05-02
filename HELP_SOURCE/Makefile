
.SUFFIXES: .o .c .cxx .source .hlp .xml .dtd .html .depend .ps.gz .pdf.gz
.PHONY: prepare all dirs generated xml dump ps pdf nonhlp_formats
.PHONY: delzerohtml interdepends google_logo html help clean tools depends depend.init

# --------------------------------------------------------------------------------

TEST_ONE:=0# default is 0; if 1 -> only one .hlp file is processed. see below .@TEST_ONE
ONLY_DO_UNITTEST:=0# default is 0; if 1 -> only unittest.hlp is processed (to make unit-test auto-update work) #@@@ set to 0 before checkin

# --------------------------------------------------------------------------------
# directories of source/destination files:

HLP_SOURCE=oldhelp
HLP_GENERATED=genhelp
XML_LOCATION=Xml
HTML_LOCATION=../lib/help_html
HLP_DEST=../lib/help
HELP_INDEX_NAME = help_index
DOTMAP=./help_map.dot
GIFMAP=./help_map.gif

# needed tools:
TOOL_OBJECTS = \
	 arb_help2xml.o

BINARIES = $(TOOL_OBJECTS:%.o=$(ARBHOME)/bin/%)

QUIETLY = ./quietly.pl

# using xsltproc and XMLLINT:
XSLTPROC = ./arb_xslt.sh
VALIDATE = $(QUIETLY) XMLLINT

# --------------------------------------------------------------------------------

$(MAIN) : all

prepare: date generated dirs tools nonhlp_formats delzerohtml

all: prepare
		@echo --------------------------------------------------- [old hlp 2 xml]
		time $(MAKE) xml
		@echo --------------------------------------------------- [interdependencies]
		time $(MAKE) interdepends
		@echo --------------------------------------------------- [xml 2 hlp]
		time $(MAKE) "USE_GENERATED_DEPENDS=1" help
		$(MAKE) $(HLP_DEST)/FORM.hlp
		@echo --------------------------------------------------- [xml 2 html]
		time $(MAKE) "USE_GENERATED_DEPENDS=1" html

$(HLP_DEST)/FORM.hlp: $(HLP_SOURCE)/FORM.hlp
		cp -p $< $@


dirs:
		mkdir -p $(XML_LOCATION)
		mkdir -p $(XML_LOCATION)/prompt

		mkdir -p $(HTML_LOCATION)
		mkdir -p $(HTML_LOCATION)/prompt

		mkdir -p $(HLP_DEST)
		mkdir -p $(HLP_DEST)/prompt

		@ln -s ../arb_help.dtd $(XML_LOCATION) || true
		@ln -s ../../arb_help.dtd $(XML_LOCATION)/prompt || true

HELP2XML=$(ARBHOME)/bin/arb_help2xml

# --------------------------------------------------------------------------------
# Generate help files in $(HLP_GENERATED):

generated: dirs
		@echo ---------------------------------------------------
		@echo ------------ Generating some help files:
		(cd $(HLP_GENERATED);$(MAKE) all)

# --------------------------------------------------------------------------------

HELP_INDEX_XML = $(XML_LOCATION)/$(HELP_INDEX_NAME).xml

ifeq ($(ONLY_DO_UNITTEST),1)
OLD_HELP_ALL= $(HLP_SOURCE)/unittest.hlp
else
OLD_HELP_ALL= \
		$(HLP_SOURCE)/arb.hlp \
		$(wildcard \
				$(HLP_GENERATED)/*.hlp \
				$(HLP_SOURCE)/*.hlp \
				$(HLP_SOURCE)/prompt/*.hlp \
		)
endif

ifeq ($(TEST_ONE),1)
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

$(XML) : $(XML_DEPEND) Makefile $(QUIETLY)

$(HELP_INDEX_XML): $(OLD_HELP_ALL) $(XML) ./generate_index.pl Makefile
	./generate_index.pl $(XML_LOCATION) $(HELP_INDEX_NAME).xml $(DOTMAP) > $@
	-dot -Tgif -o$(GIFMAP) $(DOTMAP)
	rm $(DOTMAP)

xml : $(XML) $(HELP_INDEX_XML)

dump:
		echo $(XML)

$(XML_LOCATION)/%.xml : $(HLP_SOURCE)/%.hlp $(DTD)
		@test \! -f $(HLP_GENERATED)/$(<F) || \
				( echo $<:1: exists twice -- only one existence allowed; \
				  echo $(HLP_GENERATED)/$(<F):1: other occurrence \
				  && false )
		@$(HELP2XML) $< $@ || (rm -f $@ && echo "Error: generation of $@ failed" && false)
		@$(VALIDATE) $@

$(XML_LOCATION)/%.xml : $(HLP_GENERATED)/%.hlp $(DTD)
		@test \! -f $(HLP_SOURCE)/$(<F) || \
				( echo $<:1: exists twice -- only one existence allowed; \
				  echo $(HLP_SOURCE)/$(<F):1: other occurrence \
				  && false )
		@$(HELP2XML) $< $@ || (rm -f $@ && echo "Error: generation of $@ failed" && false)
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

$(PS_DEST) : dirs
$(PDF_DEST) : dirs

nonhlp_formats: ps pdf

$(HLP_DEST)/%.ps.gz : $(HLP_SOURCE)/%.ps.gz
	cp -p $< $@

$(HLP_DEST)/%.pdf.gz : $(HLP_SOURCE)/%.pdf.gz
	cp -p $< $@

# --------------------------------------------------------------------------------

delzerohtml:
		-if [ -d $(HTML_LOCATION) ] ; then find $(HTML_LOCATION) -name "*.html" -size -1 -exec rm {} \; ; fi

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
STYLE_DEPENDS=to_depends.xsl

XSLTPROC_ARGS=--nonet --nomkdir --stringparam xml_location "$(XML_LOCATION)"

# --------------------------------------------------------------------------------

INTERDEPENDS=.depends
INTERDEPENDS_SOURCE=$(XML)
XML2DEPENDS=./xml_2_depends.pl

$(INTERDEPENDS): $(INTERDEPENDS_SOURCE) $(XML2DEPENDS) Makefile $(STYLE_DEPENDS)
	@( ( echo "$(INTERDEPENDS_SOURCE)" | $(XML2DEPENDS) $(XSLTPROC) $(STYLE_DEPENDS) $(XSLTPROC_ARGS) >$@ ) 2>interdep.err && \
	   ( $(XML2DEPENDS) CHECK $@ ) 2>>interdep.err && \
	   rm interdep.err ) || \
         (cat interdep.err; false)

interdepends: $(INTERDEPENDS)

ifeq ($(USE_GENERATED_DEPENDS),1)
include $(INTERDEPENDS)
endif

# --------------------------------------------------------------------------------

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
	-rm -f $(TOOL_OBJECTS) $(BINARIES)
	-rm -f $(XML_DEPEND) dummy date.xsl
	-rm -rf $(XML_LOCATION) $(HLP_DEST) $(HTML_LOCATION) $(INTERDEPENDS)
	$(MAKE) -C $(HLP_GENERATED) clean

# --------------------------------------------------------------------------------

tools: $(BINARIES)

LIBS=../XML/XML.a

$(HELP2XML): $(LIBS)

$(XML_DEPEND) : arb_help2xml.cxx
	touch $(XML_DEPEND)

%.o: %.cxx
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

$(HELP2XML): arb_help2xml.o $(LIBS) $(use_ARB_main)
	$(LINK_EXECUTABLE) $@ $(use_ARB_main) $< $(LIBS) $(LIBPATH) $(EXECLIBS) $(CORE_LIB) 


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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

arb_help2xml.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/arb_defs.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/arb_diff.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/arb_msg.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/attributes.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/cxxforward.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/gccver.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/static_assert.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/test_global.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/test_unit.h
arb_help2xml.o: $(ARBHOME)/INCLUDE/xml.hxx
