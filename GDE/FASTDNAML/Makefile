.SUFFIXES: .o .c .depend

OBJECT = fastDNAml.o
TARGET = $(ARBHOME)/bin/fastdnaml

all : $(TARGET)

$(TARGET) : $(OBJECT)
	$(ACC) $(cflags) -o $@ $< $(LIB) -lm

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

# DO NOT DELETE

fastDNAml.o: fastDNAml.h
