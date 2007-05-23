# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .depend
OBJECTS = ORS_S_main.o ORS_S_userdb.o ORS_S_probedb.o ORS_S_lib.o ORS_S_cf.o ORS_lib.o

$(MAIN): ors_s_proto.hxx $(OBJECTS)
	$(AR) $(MAIN) $(OBJECTS) $(CRYPTLIB)
.c.o:
	$(ACC) $(cflags) -c $<  $(AINCLUDES)
.cxx.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)

ors_s_proto.hxx: $(OBJECTS:.o=.cxx)
	../MAKEBIN/aisc_mkpt -E $(OBJECTS:.o=.cxx) >ors_s_proto.hxx.tmp
	../SOURCE_TOOLS/mv_if_diff ors_s_proto.hxx.tmp ors_s_proto.hxx

proto: ors_s_proto.hxx

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

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

ORS_lib.o: ors_lib.h
ORS_lib.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_lib.o: $(ARBHOME)/INCLUDE/arb_assert.h
ORS_lib.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_lib.o: $(ARBHOME)/INCLUDE/client.h
ORS_lib.o: $(ARBHOME)/INCLUDE/ors_client.h
ORS_lib.o: $(ARBHOME)/INCLUDE/servercntrl.h

ORS_S_cf.o: ors_lib.h
ORS_S_cf.o: ors_s.h
ORS_S_cf.o: ors_s_common.hxx
ORS_S_cf.o: ors_s_proto.hxx
ORS_S_cf.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_S_cf.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ORS_S_cf.o: $(ARBHOME)/INCLUDE/arb_assert.h
ORS_S_cf.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_S_cf.o: $(ARBHOME)/INCLUDE/client.h
ORS_S_cf.o: $(ARBHOME)/INCLUDE/ors_client.h
ORS_S_cf.o: $(ARBHOME)/INCLUDE/ors_prototypes.h
ORS_S_cf.o: $(ARBHOME)/INCLUDE/ors_server.h
ORS_S_cf.o: $(ARBHOME)/INCLUDE/server.h
ORS_S_cf.o: $(ARBHOME)/INCLUDE/servercntrl.h
ORS_S_cf.o: $(ARBHOME)/INCLUDE/struct_man.h

ORS_S_lib.o: ors_lib.h
ORS_S_lib.o: ors_s_common.hxx
ORS_S_lib.o: ors_s_proto.hxx
ORS_S_lib.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_S_lib.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ORS_S_lib.o: $(ARBHOME)/INCLUDE/arb_assert.h
ORS_S_lib.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_S_lib.o: $(ARBHOME)/INCLUDE/arbdbt.h
ORS_S_lib.o: $(ARBHOME)/INCLUDE/ors_server.h

ORS_S_main.o: ors_lib.h
ORS_S_main.o: ors_s.h
ORS_S_main.o: ors_s_common.hxx
ORS_S_main.o: ors_s_proto.hxx
ORS_S_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_S_main.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ORS_S_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
ORS_S_main.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_S_main.o: $(ARBHOME)/INCLUDE/client.h
ORS_S_main.o: $(ARBHOME)/INCLUDE/ors_client.h
ORS_S_main.o: $(ARBHOME)/INCLUDE/ors_prototypes.h
ORS_S_main.o: $(ARBHOME)/INCLUDE/ors_server.h
ORS_S_main.o: $(ARBHOME)/INCLUDE/server.h
ORS_S_main.o: $(ARBHOME)/INCLUDE/servercntrl.h
ORS_S_main.o: $(ARBHOME)/INCLUDE/struct_man.h

ORS_S_probedb.o: ors_lib.h
ORS_S_probedb.o: ors_s.h
ORS_S_probedb.o: ors_s_common.hxx
ORS_S_probedb.o: ors_s_proto.hxx
ORS_S_probedb.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_S_probedb.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ORS_S_probedb.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ORS_S_probedb.o: $(ARBHOME)/INCLUDE/arb_assert.h
ORS_S_probedb.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_S_probedb.o: $(ARBHOME)/INCLUDE/arbdbt.h
ORS_S_probedb.o: $(ARBHOME)/INCLUDE/ors_client.h
ORS_S_probedb.o: $(ARBHOME)/INCLUDE/ors_prototypes.h
ORS_S_probedb.o: $(ARBHOME)/INCLUDE/ors_server.h
ORS_S_probedb.o: $(ARBHOME)/INCLUDE/server.h
ORS_S_probedb.o: $(ARBHOME)/INCLUDE/servercntrl.h
ORS_S_probedb.o: $(ARBHOME)/INCLUDE/struct_man.h

ORS_S_userdb.o: ors_lib.h
ORS_S_userdb.o: ors_s_common.hxx
ORS_S_userdb.o: ors_s_proto.hxx
ORS_S_userdb.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_S_userdb.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ORS_S_userdb.o: $(ARBHOME)/INCLUDE/arb_assert.h
ORS_S_userdb.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_S_userdb.o: $(ARBHOME)/INCLUDE/arbdbt.h
ORS_S_userdb.o: $(ARBHOME)/INCLUDE/ors_server.h
