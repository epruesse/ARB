# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .depend
CPP_OBJECTS =	ORS_C_main.o ORS_C_PT.o ORS_C_html.o ORS_C_java.o \
		ORS_C_user.o ORS_C_lib.o ORS_C_java_main.o ORS_C_scripts.o \
		ORS_C_probe.o ORS_lib.o ORS_C_rtok.o
$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)
.cxx.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)

proto:
	rm -f ors_c_proto.hxx
	../MAKEBIN/aisc_mkpt $(CPP_OBJECTS:.o=.cxx) | grep -v '::' >ors_c_proto.hxx

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

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

ORS_C_html.o: ors_c.hxx
ORS_C_html.o: ors_c_java.hxx
ORS_C_html.o: ors_c_proto.hxx
ORS_C_html.o: ors_lib.h
ORS_C_html.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_html.o: $(ARBHOME)/INCLUDE/arb_assert.h
ORS_C_html.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_html.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ORS_C_html.o: $(ARBHOME)/INCLUDE/client.h
ORS_C_html.o: $(ARBHOME)/INCLUDE/servercntrl.h

ORS_C_java.o: ors_c.hxx
ORS_C_java.o: ors_c_java.hxx
ORS_C_java.o: ors_c_proto.hxx
ORS_C_java.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_java.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ORS_C_java.o: $(ARBHOME)/INCLUDE/arb_assert.h
ORS_C_java.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_java.o: $(ARBHOME)/INCLUDE/arbdbt.h
ORS_C_java.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ORS_C_java.o: $(ARBHOME)/INCLUDE/client.h

ORS_C_java_main.o: ors_c.hxx
ORS_C_java_main.o: ors_c_java.hxx
ORS_C_java_main.o: ors_c_proto.hxx
ORS_C_java_main.o: ors_lib.h
ORS_C_java_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_java_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ORS_C_java_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
ORS_C_java_main.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_java_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
ORS_C_java_main.o: $(ARBHOME)/INCLUDE/cat_tree.hxx

ORS_C_lib.o: ors_c.hxx
ORS_C_lib.o: ors_c_java.hxx
ORS_C_lib.o: ors_c_proto.hxx
ORS_C_lib.o: ors_lib.h
ORS_C_lib.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_lib.o: $(ARBHOME)/INCLUDE/arb_assert.h
ORS_C_lib.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_lib.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ORS_C_lib.o: $(ARBHOME)/INCLUDE/client.h
ORS_C_lib.o: $(ARBHOME)/INCLUDE/servercntrl.h

ORS_C_main.o: ors_c.hxx
ORS_C_main.o: ors_c_proto.hxx
ORS_C_main.o: ors_lib.h
ORS_C_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
ORS_C_main.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_main.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ORS_C_main.o: $(ARBHOME)/INCLUDE/client.h
ORS_C_main.o: $(ARBHOME)/INCLUDE/servercntrl.h

ORS_C_probe.o: ors_c.hxx
ORS_C_probe.o: ors_c_proto.hxx
ORS_C_probe.o: ors_lib.h
ORS_C_probe.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_probe.o: $(ARBHOME)/INCLUDE/arb_assert.h
ORS_C_probe.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_probe.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ORS_C_probe.o: $(ARBHOME)/INCLUDE/client.h
ORS_C_probe.o: $(ARBHOME)/INCLUDE/servercntrl.h

ORS_C_PT.o: ors_c.hxx
ORS_C_PT.o: ors_c_proto.hxx
ORS_C_PT.o: ors_lib.h
ORS_C_PT.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_PT.o: $(ARBHOME)/INCLUDE/arb_assert.h
ORS_C_PT.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_PT.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ORS_C_PT.o: $(ARBHOME)/INCLUDE/client.h
ORS_C_PT.o: $(ARBHOME)/INCLUDE/servercntrl.h

ORS_C_rtok.o: ors_c.hxx
ORS_C_rtok.o: ors_c_java.hxx
ORS_C_rtok.o: ors_c_proto.hxx
ORS_C_rtok.o: ors_lib.h
ORS_C_rtok.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_rtok.o: $(ARBHOME)/INCLUDE/arb_assert.h
ORS_C_rtok.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_rtok.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ORS_C_rtok.o: $(ARBHOME)/INCLUDE/client.h
ORS_C_rtok.o: $(ARBHOME)/INCLUDE/servercntrl.h

ORS_C_scripts.o: ors_c.hxx
ORS_C_scripts.o: ors_c_java.hxx
ORS_C_scripts.o: ors_c_proto.hxx
ORS_C_scripts.o: ors_lib.h
ORS_C_scripts.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_scripts.o: $(ARBHOME)/INCLUDE/arb_assert.h
ORS_C_scripts.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_scripts.o: $(ARBHOME)/INCLUDE/cat_tree.hxx

ORS_C_user.o: ors_c.hxx
ORS_C_user.o: ors_c_java.hxx
ORS_C_user.o: ors_c_proto.hxx
ORS_C_user.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_user.o: $(ARBHOME)/INCLUDE/arb_assert.h
ORS_C_user.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_user.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ORS_C_user.o: $(ARBHOME)/INCLUDE/client.h
ORS_C_user.o: $(ARBHOME)/INCLUDE/servercntrl.h

ORS_lib.o: ors_lib.h
ORS_lib.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_lib.o: $(ARBHOME)/INCLUDE/arb_assert.h
ORS_lib.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_lib.o: $(ARBHOME)/INCLUDE/client.h
ORS_lib.o: $(ARBHOME)/INCLUDE/servercntrl.h
