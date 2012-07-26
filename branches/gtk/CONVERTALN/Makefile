.SUFFIXES: .o .a .cxx .depend

OBJECTS= $(subst .cxx,.o,$(wildcard *.cxx))

$(MAIN): $(OBJECTS) check_error_codes
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

check_error_codes:
	@./show_error_list.pl -check

.cxx.o:
	$(CPP) $(cflags) -c $< $(AINCLUDES) $(POST_COMPILE)

proto:
	../AISC_MKPTPS/aisc_mkpt -P -G -A -w prototypes.h $(OBJECTS:.o=.cxx) >prototypes.h.tmp
	../SOURCE_TOOLS/mv_if_diff prototypes.h.tmp prototypes.h

clean:
	rm -f *.o $(BINARY)

DEPENDS = $(OBJECTS:.o=.depend)
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
