.SUFFIXES: .o .c .depend

OBJECT        = fastDNAml.o
FASTDNAML_DOC = fastDNAml.doc
TARGET		  = $(ARBHOME)/bin/fastdnaml

all : $(TARGET) doc

$(TARGET) : $(OBJECT)
	$(ACC) $(cflags) -o $@ $< $(LIB) -lm
	$(MAKE) doc

.c.o:
	$(ACC) $(cflags) -c -o $@ $< $(AINCLUDES)

DEPENDS = $(OBJECT:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@


ARB_DOC=../../GDEHELP/fastdnaml.help
ARB_DOC_PREFIX=arb_help_prefix.txt

doc:  $(ARB_DOC)

$(ARB_DOC) : $(ARB_DOC_PREFIX) $(FASTDNAML_DOC)
		cat $(ARB_DOC_PREFIX) $(FASTDNAML_DOC) >$@

# DO NOT DELETE

fastDNAml.o: fastDNAml.h
