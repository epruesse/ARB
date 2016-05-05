# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .cxx .depend

CPP_OBJECTS = sax2Handler.o \
		xmlParser.o \

TARGET=$(ARBHOME)/bin/xml2newick

$(MAIN): $(TARGET)

$(TARGET) : $(CPP_OBJECTS)
	$(LINK_EXECUTABLE) $@ $(CPP_OBJECTS) $(EXECLIBS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c -o $@ $<  $(CXX_INCLUDES) $(POST_COMPILE) $<

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
