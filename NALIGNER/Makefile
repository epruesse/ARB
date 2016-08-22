# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

CPP_OBJECTS = \
	ali_aligner.o \
	ali_arbdb.o \
	ali_global.o \
	ali_main.o \
	ali_pathmap.o \
	ali_prealigner.o \
	ali_profile.o \
	ali_pt.o \
	ali_sequence.o \
	ali_solution.o \

$(MAIN): $(CPP_OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c -o $@ $< $(CXX_INCLUDES) $(POST_COMPILE)

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

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

ali_aligner.o: ali_aligner.hxx
ali_aligner.o: ali_arbdb.hxx
ali_aligner.o: ali_misc.hxx
ali_aligner.o: ali_other_stuff.hxx
ali_aligner.o: ali_pathmap.hxx
ali_aligner.o: ali_profile.hxx
ali_aligner.o: ali_pt.hxx
ali_aligner.o: ali_sequence.hxx
ali_aligner.o: ali_solution.hxx
ali_aligner.o: ali_tarray.hxx
ali_aligner.o: ali_tlist.hxx
ali_aligner.o: ali_tstack.hxx
ali_aligner.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_aligner.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_aligner.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ali_aligner.o: $(ARBHOME)/INCLUDE/aisc_global.h
ali_aligner.o: $(ARBHOME)/INCLUDE/arb_assert.h
ali_aligner.o: $(ARBHOME)/INCLUDE/arb_core.h
ali_aligner.o: $(ARBHOME)/INCLUDE/arb_error.h
ali_aligner.o: $(ARBHOME)/INCLUDE/arb_msg.h
ali_aligner.o: $(ARBHOME)/INCLUDE/arb_string.h
ali_aligner.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_aligner.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ali_aligner.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_aligner.o: $(ARBHOME)/INCLUDE/arbtools.h
ali_aligner.o: $(ARBHOME)/INCLUDE/attributes.h
ali_aligner.o: $(ARBHOME)/INCLUDE/bytestring.h
ali_aligner.o: $(ARBHOME)/INCLUDE/client.h
ali_aligner.o: $(ARBHOME)/INCLUDE/client_types.h
ali_aligner.o: $(ARBHOME)/INCLUDE/cxxforward.h
ali_aligner.o: $(ARBHOME)/INCLUDE/downcast.h
ali_aligner.o: $(ARBHOME)/INCLUDE/dupstr.h
ali_aligner.o: $(ARBHOME)/INCLUDE/gccver.h
ali_aligner.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_aligner.o: $(ARBHOME)/INCLUDE/server.h
ali_aligner.o: $(ARBHOME)/INCLUDE/servercntrl.h
ali_aligner.o: $(ARBHOME)/INCLUDE/smartptr.h
ali_aligner.o: $(ARBHOME)/INCLUDE/static_assert.h
ali_aligner.o: $(ARBHOME)/INCLUDE/test_global.h

ali_arbdb.o: ali_arbdb.hxx
ali_arbdb.o: ali_misc.hxx
ali_arbdb.o: ali_other_stuff.hxx
ali_arbdb.o: ali_sequence.hxx
ali_arbdb.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/aisc_global.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/arb_assert.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/arb_core.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/arb_error.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/arb_msg.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/arb_string.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/arbtools.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/attributes.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/bytestring.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/cxxforward.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/downcast.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/dupstr.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/gccver.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/server.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/servercntrl.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/smartptr.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/static_assert.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/test_global.h

ali_global.o: ali_arbdb.hxx
ali_global.o: ali_global.hxx
ali_global.o: ali_misc.hxx
ali_global.o: ali_other_stuff.hxx
ali_global.o: ali_pathmap.hxx
ali_global.o: ali_prealigner.hxx
ali_global.o: ali_profile.hxx
ali_global.o: ali_pt.hxx
ali_global.o: ali_sequence.hxx
ali_global.o: ali_solution.hxx
ali_global.o: ali_tarray.hxx
ali_global.o: ali_tlist.hxx
ali_global.o: ali_tstack.hxx
ali_global.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_global.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_global.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ali_global.o: $(ARBHOME)/INCLUDE/aisc_global.h
ali_global.o: $(ARBHOME)/INCLUDE/arb_assert.h
ali_global.o: $(ARBHOME)/INCLUDE/arb_core.h
ali_global.o: $(ARBHOME)/INCLUDE/arb_error.h
ali_global.o: $(ARBHOME)/INCLUDE/arb_msg.h
ali_global.o: $(ARBHOME)/INCLUDE/arb_string.h
ali_global.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_global.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ali_global.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_global.o: $(ARBHOME)/INCLUDE/arbtools.h
ali_global.o: $(ARBHOME)/INCLUDE/attributes.h
ali_global.o: $(ARBHOME)/INCLUDE/bytestring.h
ali_global.o: $(ARBHOME)/INCLUDE/client.h
ali_global.o: $(ARBHOME)/INCLUDE/client_types.h
ali_global.o: $(ARBHOME)/INCLUDE/cxxforward.h
ali_global.o: $(ARBHOME)/INCLUDE/downcast.h
ali_global.o: $(ARBHOME)/INCLUDE/dupstr.h
ali_global.o: $(ARBHOME)/INCLUDE/gccver.h
ali_global.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_global.o: $(ARBHOME)/INCLUDE/server.h
ali_global.o: $(ARBHOME)/INCLUDE/servercntrl.h
ali_global.o: $(ARBHOME)/INCLUDE/smartptr.h
ali_global.o: $(ARBHOME)/INCLUDE/static_assert.h
ali_global.o: $(ARBHOME)/INCLUDE/test_global.h

ali_main.o: ali_arbdb.hxx
ali_main.o: ali_global.hxx
ali_main.o: ali_misc.hxx
ali_main.o: ali_other_stuff.hxx
ali_main.o: ali_pathmap.hxx
ali_main.o: ali_prealigner.hxx
ali_main.o: ali_profile.hxx
ali_main.o: ali_pt.hxx
ali_main.o: ali_sequence.hxx
ali_main.o: ali_solution.hxx
ali_main.o: ali_tarray.hxx
ali_main.o: ali_tlist.hxx
ali_main.o: ali_tstack.hxx
ali_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_main.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ali_main.o: $(ARBHOME)/INCLUDE/aisc_global.h
ali_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
ali_main.o: $(ARBHOME)/INCLUDE/arb_core.h
ali_main.o: $(ARBHOME)/INCLUDE/arb_error.h
ali_main.o: $(ARBHOME)/INCLUDE/arb_msg.h
ali_main.o: $(ARBHOME)/INCLUDE/arb_string.h
ali_main.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_main.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ali_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_main.o: $(ARBHOME)/INCLUDE/arbtools.h
ali_main.o: $(ARBHOME)/INCLUDE/attributes.h
ali_main.o: $(ARBHOME)/INCLUDE/bytestring.h
ali_main.o: $(ARBHOME)/INCLUDE/client.h
ali_main.o: $(ARBHOME)/INCLUDE/client_types.h
ali_main.o: $(ARBHOME)/INCLUDE/cxxforward.h
ali_main.o: $(ARBHOME)/INCLUDE/downcast.h
ali_main.o: $(ARBHOME)/INCLUDE/dupstr.h
ali_main.o: $(ARBHOME)/INCLUDE/gccver.h
ali_main.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_main.o: $(ARBHOME)/INCLUDE/server.h
ali_main.o: $(ARBHOME)/INCLUDE/servercntrl.h
ali_main.o: $(ARBHOME)/INCLUDE/smartptr.h
ali_main.o: $(ARBHOME)/INCLUDE/static_assert.h
ali_main.o: $(ARBHOME)/INCLUDE/test_global.h

ali_pathmap.o: ali_misc.hxx
ali_pathmap.o: ali_pathmap.hxx
ali_pathmap.o: ali_tarray.hxx
ali_pathmap.o: ali_tlist.hxx
ali_pathmap.o: $(ARBHOME)/INCLUDE/arbtools.h
ali_pathmap.o: $(ARBHOME)/INCLUDE/attributes.h
ali_pathmap.o: $(ARBHOME)/INCLUDE/cxxforward.h
ali_pathmap.o: $(ARBHOME)/INCLUDE/gccver.h

ali_prealigner.o: ali_aligner.hxx
ali_prealigner.o: ali_arbdb.hxx
ali_prealigner.o: ali_misc.hxx
ali_prealigner.o: ali_other_stuff.hxx
ali_prealigner.o: ali_pathmap.hxx
ali_prealigner.o: ali_prealigner.hxx
ali_prealigner.o: ali_profile.hxx
ali_prealigner.o: ali_pt.hxx
ali_prealigner.o: ali_sequence.hxx
ali_prealigner.o: ali_solution.hxx
ali_prealigner.o: ali_tarray.hxx
ali_prealigner.o: ali_tlist.hxx
ali_prealigner.o: ali_tstack.hxx
ali_prealigner.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/aisc_global.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/arb_assert.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/arb_core.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/arb_error.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/arb_msg.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/arb_string.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/arbtools.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/attributes.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/bytestring.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/client.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/client_types.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/cxxforward.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/downcast.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/dupstr.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/gccver.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/server.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/servercntrl.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/smartptr.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/static_assert.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/test_global.h

ali_profile.o: ali_arbdb.hxx
ali_profile.o: ali_misc.hxx
ali_profile.o: ali_other_stuff.hxx
ali_profile.o: ali_profile.hxx
ali_profile.o: ali_pt.hxx
ali_profile.o: ali_sequence.hxx
ali_profile.o: ali_tlist.hxx
ali_profile.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_profile.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_profile.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ali_profile.o: $(ARBHOME)/INCLUDE/aisc_global.h
ali_profile.o: $(ARBHOME)/INCLUDE/arb_assert.h
ali_profile.o: $(ARBHOME)/INCLUDE/arb_core.h
ali_profile.o: $(ARBHOME)/INCLUDE/arb_error.h
ali_profile.o: $(ARBHOME)/INCLUDE/arb_msg.h
ali_profile.o: $(ARBHOME)/INCLUDE/arb_string.h
ali_profile.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_profile.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ali_profile.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_profile.o: $(ARBHOME)/INCLUDE/arbtools.h
ali_profile.o: $(ARBHOME)/INCLUDE/attributes.h
ali_profile.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
ali_profile.o: $(ARBHOME)/INCLUDE/bytestring.h
ali_profile.o: $(ARBHOME)/INCLUDE/client.h
ali_profile.o: $(ARBHOME)/INCLUDE/client_types.h
ali_profile.o: $(ARBHOME)/INCLUDE/cxxforward.h
ali_profile.o: $(ARBHOME)/INCLUDE/downcast.h
ali_profile.o: $(ARBHOME)/INCLUDE/dupstr.h
ali_profile.o: $(ARBHOME)/INCLUDE/gccver.h
ali_profile.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_profile.o: $(ARBHOME)/INCLUDE/server.h
ali_profile.o: $(ARBHOME)/INCLUDE/servercntrl.h
ali_profile.o: $(ARBHOME)/INCLUDE/smartptr.h
ali_profile.o: $(ARBHOME)/INCLUDE/static_assert.h
ali_profile.o: $(ARBHOME)/INCLUDE/test_global.h

ali_pt.o: ali_misc.hxx
ali_pt.o: ali_other_stuff.hxx
ali_pt.o: ali_pt.hxx
ali_pt.o: ali_sequence.hxx
ali_pt.o: ali_tlist.hxx
ali_pt.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_pt.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_pt.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ali_pt.o: $(ARBHOME)/INCLUDE/aisc_global.h
ali_pt.o: $(ARBHOME)/INCLUDE/arb_assert.h
ali_pt.o: $(ARBHOME)/INCLUDE/arb_core.h
ali_pt.o: $(ARBHOME)/INCLUDE/arb_error.h
ali_pt.o: $(ARBHOME)/INCLUDE/arb_msg.h
ali_pt.o: $(ARBHOME)/INCLUDE/arb_string.h
ali_pt.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_pt.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ali_pt.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_pt.o: $(ARBHOME)/INCLUDE/arbtools.h
ali_pt.o: $(ARBHOME)/INCLUDE/attributes.h
ali_pt.o: $(ARBHOME)/INCLUDE/bytestring.h
ali_pt.o: $(ARBHOME)/INCLUDE/client.h
ali_pt.o: $(ARBHOME)/INCLUDE/client_types.h
ali_pt.o: $(ARBHOME)/INCLUDE/cxxforward.h
ali_pt.o: $(ARBHOME)/INCLUDE/downcast.h
ali_pt.o: $(ARBHOME)/INCLUDE/dupstr.h
ali_pt.o: $(ARBHOME)/INCLUDE/gccver.h
ali_pt.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_pt.o: $(ARBHOME)/INCLUDE/server.h
ali_pt.o: $(ARBHOME)/INCLUDE/servercntrl.h
ali_pt.o: $(ARBHOME)/INCLUDE/smartptr.h
ali_pt.o: $(ARBHOME)/INCLUDE/static_assert.h
ali_pt.o: $(ARBHOME)/INCLUDE/test_global.h

ali_sequence.o: ali_misc.hxx
ali_sequence.o: ali_sequence.hxx
ali_sequence.o: $(ARBHOME)/INCLUDE/arbtools.h
ali_sequence.o: $(ARBHOME)/INCLUDE/attributes.h
ali_sequence.o: $(ARBHOME)/INCLUDE/cxxforward.h
ali_sequence.o: $(ARBHOME)/INCLUDE/gccver.h

ali_solution.o: ali_arbdb.hxx
ali_solution.o: ali_misc.hxx
ali_solution.o: ali_other_stuff.hxx
ali_solution.o: ali_profile.hxx
ali_solution.o: ali_pt.hxx
ali_solution.o: ali_sequence.hxx
ali_solution.o: ali_solution.hxx
ali_solution.o: ali_tlist.hxx
ali_solution.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_solution.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_solution.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ali_solution.o: $(ARBHOME)/INCLUDE/aisc_global.h
ali_solution.o: $(ARBHOME)/INCLUDE/arb_assert.h
ali_solution.o: $(ARBHOME)/INCLUDE/arb_core.h
ali_solution.o: $(ARBHOME)/INCLUDE/arb_error.h
ali_solution.o: $(ARBHOME)/INCLUDE/arb_msg.h
ali_solution.o: $(ARBHOME)/INCLUDE/arb_string.h
ali_solution.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_solution.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ali_solution.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_solution.o: $(ARBHOME)/INCLUDE/arbtools.h
ali_solution.o: $(ARBHOME)/INCLUDE/attributes.h
ali_solution.o: $(ARBHOME)/INCLUDE/bytestring.h
ali_solution.o: $(ARBHOME)/INCLUDE/client.h
ali_solution.o: $(ARBHOME)/INCLUDE/client_types.h
ali_solution.o: $(ARBHOME)/INCLUDE/cxxforward.h
ali_solution.o: $(ARBHOME)/INCLUDE/downcast.h
ali_solution.o: $(ARBHOME)/INCLUDE/dupstr.h
ali_solution.o: $(ARBHOME)/INCLUDE/gccver.h
ali_solution.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_solution.o: $(ARBHOME)/INCLUDE/server.h
ali_solution.o: $(ARBHOME)/INCLUDE/servercntrl.h
ali_solution.o: $(ARBHOME)/INCLUDE/smartptr.h
ali_solution.o: $(ARBHOME)/INCLUDE/static_assert.h
ali_solution.o: $(ARBHOME)/INCLUDE/test_global.h
