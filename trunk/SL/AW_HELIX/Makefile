# for variables passed from parent makefile see ../../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = AW_helix.o
OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

.c.o:
	$(A_CC) $(cflags) -c $< $(CC_INCLUDES) $(POST_COMPILE) $<

clean:
	rm -f $(OBJECTS) *.a

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

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from SL)

AW_helix.o: AW_helix.hxx
AW_helix.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_helix.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AW_helix.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_helix.o: $(ARBHOME)/INCLUDE/arb_core.h
AW_helix.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_helix.o: $(ARBHOME)/INCLUDE/arb_mem.h
AW_helix.o: $(ARBHOME)/INCLUDE/arb_msg.h
AW_helix.o: $(ARBHOME)/INCLUDE/arb_string.h
AW_helix.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_helix.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_helix.o: $(ARBHOME)/INCLUDE/arbdbt.h
AW_helix.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_helix.o: $(ARBHOME)/INCLUDE/attributes.h
AW_helix.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
AW_helix.o: $(ARBHOME)/INCLUDE/aw_base.hxx
AW_helix.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AW_helix.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AW_helix.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AW_helix.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AW_helix.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AW_helix.o: $(ARBHOME)/INCLUDE/awt_config_manager.hxx
AW_helix.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
AW_helix.o: $(ARBHOME)/INCLUDE/cb.h
AW_helix.o: $(ARBHOME)/INCLUDE/cb_base.h
AW_helix.o: $(ARBHOME)/INCLUDE/cbtypes.h
AW_helix.o: $(ARBHOME)/INCLUDE/cxxforward.h
AW_helix.o: $(ARBHOME)/INCLUDE/downcast.h
AW_helix.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_helix.o: $(ARBHOME)/INCLUDE/gccver.h
AW_helix.o: $(ARBHOME)/INCLUDE/smartptr.h
AW_helix.o: $(ARBHOME)/INCLUDE/static_assert.h
AW_helix.o: $(ARBHOME)/INCLUDE/test_global.h
AW_helix.o: $(ARBHOME)/INCLUDE/ttypes.h
