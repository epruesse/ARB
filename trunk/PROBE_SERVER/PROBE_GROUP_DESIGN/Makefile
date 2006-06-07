# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .depend

# --------------------------------------------------------------------------------

# --------------------------------------------------------------------------------

CPP_OBJECTS = pgd_main.o
#probe_search.o

all :
	$(CLEAN_BEFORE_MAKE)
	$(MAKE) $(MAIN)

$(MAIN): $(CPP_OBJECTS)
	$(XAR) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c -o $@ $(@:.o=.cxx)  $(CPPINCLUDES)

clean:
	-rm *.a *.o
	-rm -r SunWS_cache


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

pgd_main.o: ../common.h
pgd_main.o: ../global_defs.h
pgd_main.o: ../mapping.h
pgd_main.o: ../path_code.h
pgd_main.o: ../read_config.h
pgd_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
pgd_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
pgd_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
pgd_main.o: $(ARBHOME)/INCLUDE/arbdb.h
pgd_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
pgd_main.o: $(ARBHOME)/INCLUDE/client.h
pgd_main.o: $(ARBHOME)/INCLUDE/config_parser.h
pgd_main.o: $(ARBHOME)/INCLUDE/output.h
pgd_main.o: $(ARBHOME)/INCLUDE/PT_com.h
pgd_main.o: $(ARBHOME)/INCLUDE/servercntrl.h
