.SUFFIXES: .o .c

OBJECT        = fastDNAml_1_2_1.o
FASTDNAML_DOC = fastDNAml_doc_1.2.txt

$(ARBHOME)/bin/fastdnaml: $(OBJECT) doc
	$(ACC) $(cflags) -o $@ $< $(LIB) -lm

.c.o:
	$(ACC) $(cflags) -c -o $@ $< $(AINCLUDES)

depend:
	makedepend $(MAKEDEPENDINC) $(OBJECT:.o=.c)

ARB_DOC=../../GDEHELP/fastdnaml.help
ARB_DOC_PREFIX=arb_help_prefix.txt

doc:  $(ARB_DOC)

$(ARB_DOC) : $(ARB_DOC_PREFIX) $(FASTDNAML_DOC)
		cat $(ARB_DOC_PREFIX) $(FASTDNAML_DOC) >$@

# DO NOT DELETE

fastDNAml_1_2_1.o:  fastDNAml_1_2_1.h
