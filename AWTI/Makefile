# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .depend

CPP_OBJECTS = AWTI_import.o \
		      AWTI_export.o \

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)
.cxx.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

AWTI_export.o: awti_exp_local.hxx
AWTI_export.o: awti_export.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTI_export.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWTI_export.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWTI_export.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTI_export.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWTI_export.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/aw_global.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/awt.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
AWTI_export.o: $(ARBHOME)/INCLUDE/inline.h
AWTI_export.o: $(ARBHOME)/INCLUDE/xml.hxx

AWTI_import.o: awti_imp_local.hxx
AWTI_import.o: awti_import.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/ad_prot.h
AWTI_import.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
AWTI_import.o: $(ARBHOME)/INCLUDE/adGene.h
AWTI_import.o: $(ARBHOME)/INCLUDE/arb_assert.h
AWTI_import.o: $(ARBHOME)/INCLUDE/arbdb.h
AWTI_import.o: $(ARBHOME)/INCLUDE/arbdbt.h
AWTI_import.o: $(ARBHOME)/INCLUDE/aw_device.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/aw_global.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/aw_root.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/aw_window.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/awt.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/awt_advice.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/awt_changekey.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
AWTI_import.o: $(ARBHOME)/INCLUDE/GAGenomImport.h
AWTI_import.o: $(ARBHOME)/INCLUDE/GEN.hxx
