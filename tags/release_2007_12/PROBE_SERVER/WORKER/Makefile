# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .depend

# --------------------------------------------------------------------------------

# --------------------------------------------------------------------------------

CPP_OBJECTS = psw_main.o

all : $(MAIN)

$(MAIN): $(CPP_OBJECTS)
	$(XAR) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c -o $@ $(@:.o=.cxx)  $(CPPINCLUDES)

clean:
	rm -f $(CPP_OBJECTS) *.a

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

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

psw_main.o: ../common.h
psw_main.o: ../global_defs.h
psw_main.o: ../mapping.h
psw_main.o: ../path_code.h
psw_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
psw_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
psw_main.o: $(ARBHOME)/INCLUDE/arbdb.h
psw_main.o: $(ARBHOME)/INCLUDE/smartptr.h
