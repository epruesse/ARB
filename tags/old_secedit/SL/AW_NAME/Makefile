# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS =
CPP_OBJECTS = \
	AW_rename.o \
	AW_names_admin.o \

OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(AR) $(MAIN) $(OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES)

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
AW_names_admin.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/awt.hxx
AW_names_admin.o: $(ARBHOME)/INCLUDE/awt_advice.hxx

AW_rename.o: AW_rename.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/ad_prot.h
AW_rename.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AW_rename.o: $(ARBHOME)/INCLUDE/arb_assert.h
AW_rename.o: $(ARBHOME)/INCLUDE/arbdb.h
AW_rename.o: $(ARBHOME)/INCLUDE/arbdbt.h
AW_rename.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AW_rename.o: $(ARBHOME)/INCLUDE/client.h
AW_rename.o: $(ARBHOME)/INCLUDE/inline.h
AW_rename.o: $(ARBHOME)/INCLUDE/names_client.h
AW_rename.o: $(ARBHOME)/INCLUDE/servercntrl.h
