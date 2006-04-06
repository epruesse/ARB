# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .depend

# --------------------------------------------------------------------------------

# --------------------------------------------------------------------------------

CPP_OBJECTS = pg_db.o pg_main.o pg_search.o
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

pg_db.o: pg_db.hxx
pg_db.o: pg_def.hxx
pg_db.o: $(ARBHOME)/INCLUDE/ad_prot.h
pg_db.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
pg_db.o: $(ARBHOME)/INCLUDE/arb_assert.h
pg_db.o: $(ARBHOME)/INCLUDE/arbdb.h
pg_db.o: $(ARBHOME)/INCLUDE/arbdbt.h

pg_main.o: ../common.h
pg_main.o: ../global_defs.h
pg_main.o: ../path_code.h
pg_main.o: ../read_config.h
pg_main.o: pg_db.hxx
pg_main.o: pg_def.hxx
pg_main.o: pg_search.hxx
pg_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
pg_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
pg_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
pg_main.o: $(ARBHOME)/INCLUDE/arbdb.h
pg_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
pg_main.o: $(ARBHOME)/INCLUDE/config_parser.h
pg_main.o: $(ARBHOME)/INCLUDE/output.h
pg_main.o: $(ARBHOME)/INCLUDE/smartptr.h

pg_search.o: ../global_defs.h
pg_search.o: pg_db.hxx
pg_search.o: pg_def.hxx
pg_search.o: pg_search.hxx
pg_search.o: $(ARBHOME)/INCLUDE/ad_prot.h
pg_search.o: $(ARBHOME)/INCLUDE/arb_assert.h
pg_search.o: $(ARBHOME)/INCLUDE/arbdb.h
pg_search.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
pg_search.o: $(ARBHOME)/INCLUDE/aw_device.hxx
pg_search.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
pg_search.o: $(ARBHOME)/INCLUDE/aw_root.hxx
pg_search.o: $(ARBHOME)/INCLUDE/aw_window.hxx
pg_search.o: $(ARBHOME)/INCLUDE/client.h
pg_search.o: $(ARBHOME)/INCLUDE/servercntrl.h
