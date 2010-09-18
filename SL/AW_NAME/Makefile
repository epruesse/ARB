# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS =
CPP_OBJECTS = \
	AW_rename.o \
	AW_names_admin.o \

OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES) $(POST_COMPILE)

.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES) $(POST_COMPILE)

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

AW_names_admin.o: AW_rename.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/arb_core.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/attributes.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_advice.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_edit.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/awt.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/smartptr.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/test_global.h

AW_rename.o: AW_rename.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
AW_rename.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_rename.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AW_rename.o: $(ARBHOME)/INCLUDE/aisc_global.h
AW_rename.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_rename.o: $(ARBHOME)/INCLUDE/arb_core.h
AW_rename.o: $(ARBHOME)/INCLUDE/arb_error.h
AW_rename.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_rename.o: $(ARBHOME)/INCLUDE/arbdb_base.h
AW_rename.o: $(ARBHOME)/INCLUDE/arbdbt.h
AW_rename.o: $(ARBHOME)/INCLUDE/arbtools.h
AW_rename.o: $(ARBHOME)/INCLUDE/attributes.h
AW_rename.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_position.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/bytestring.h
AW_rename.o: $(ARBHOME)/INCLUDE/client.h
AW_rename.o: $(ARBHOME)/INCLUDE/client_types.h
AW_rename.o: $(ARBHOME)/INCLUDE/dupstr.h
AW_rename.o: $(ARBHOME)/INCLUDE/names_client.h
AW_rename.o: $(ARBHOME)/INCLUDE/servercntrl.h
AW_rename.o: $(ARBHOME)/INCLUDE/smartptr.h
AW_rename.o: $(ARBHOME)/INCLUDE/test_global.h
