.SUFFIXES: .o .c

OBJECT        = fastDNAml.o
FASTDNAML_DOC = fastDNAml.doc
TARGET		  = $(ARBHOME)/bin/fastdnaml

all : $(TARGET) doc

$(TARGET) : $(OBJECT)
	$(ACC) $(cflags) -o $@ $< $(LIB) -lm
	$(MAKE) doc

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

fastDNAml.o:  fastDNAml.h
