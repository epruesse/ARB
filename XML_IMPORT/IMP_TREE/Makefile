# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

CPP_OBJECTS = MySAXHandler.o \
			  testParser.o \

TARGET=$(ARBHOME)/bin/arb_xml_parser

$(MAIN): $(TARGET)

$(TARGET) : $(CPP_OBJECTS)
	$(CPP) $(lflags) -o $@ $(CPP_OBJECTS) # @@@ should use $AR

.cxx.o:
	$(CPP) $(cflags) -c -o $@ $<  $(CPPINCLUDES)

DEPENDS = $(CPP_OBJECTS:.o=.depend)
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

# DO NOT DELETE THIS LINE -- make depend depends on it.
