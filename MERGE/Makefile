# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .depend
CPP_OBJECTS =   MG_checkfield.o MG_main.o MG_trees.o MG_alignment.o \
	MG_species.o MG_extendeds.o MG_names.o MG_gene_species.o \
	MG_configs.o MG_preserves.o

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

MG_alignment.o: merge.hxx
MG_alignment.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_alignment.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_alignment.o: $(ARBHOME)/INCLUDE/arb_assert.h
MG_alignment.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_alignment.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_alignment.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_alignment.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
MG_alignment.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_alignment.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_alignment.o: $(ARBHOME)/INCLUDE/awt.hxx
MG_alignment.o: $(ARBHOME)/INCLUDE/awt_changekey.hxx
MG_alignment.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx

MG_checkfield.o: merge.hxx
MG_checkfield.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_checkfield.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_checkfield.o: $(ARBHOME)/INCLUDE/arb_assert.h
MG_checkfield.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_checkfield.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_checkfield.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MG_checkfield.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_checkfield.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
MG_checkfield.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_checkfield.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_checkfield.o: $(ARBHOME)/INCLUDE/awt.hxx
MG_checkfield.o: $(ARBHOME)/INCLUDE/awt_changekey.hxx
MG_checkfield.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx

MG_configs.o: merge.hxx
MG_configs.o: $(ARBHOME)/INCLUDE/ad_config.h
MG_configs.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_configs.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_configs.o: $(ARBHOME)/INCLUDE/arb_assert.h
MG_configs.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_configs.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_configs.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_configs.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
MG_configs.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_configs.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_configs.o: $(ARBHOME)/INCLUDE/awt.hxx
MG_configs.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx

MG_extendeds.o: merge.hxx
MG_extendeds.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_extendeds.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_extendeds.o: $(ARBHOME)/INCLUDE/arb_assert.h
MG_extendeds.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_extendeds.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_extendeds.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MG_extendeds.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_extendeds.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
MG_extendeds.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_extendeds.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_extendeds.o: $(ARBHOME)/INCLUDE/awt.hxx
MG_extendeds.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
MG_extendeds.o: $(ARBHOME)/INCLUDE/db_scanner.hxx

MG_gene_species.o: merge.hxx
MG_gene_species.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_gene_species.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_gene_species.o: $(ARBHOME)/INCLUDE/adGene.h
MG_gene_species.o: $(ARBHOME)/INCLUDE/arb_assert.h
MG_gene_species.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_gene_species.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_gene_species.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MG_gene_species.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_gene_species.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
MG_gene_species.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_gene_species.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_gene_species.o: $(ARBHOME)/INCLUDE/awt.hxx
MG_gene_species.o: $(ARBHOME)/INCLUDE/awt_config_manager.hxx
MG_gene_species.o: $(ARBHOME)/INCLUDE/inline.h

MG_main.o: merge.hxx
MG_main.o: mg_merge.hxx
MG_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
MG_main.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_main.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MG_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_main.o: $(ARBHOME)/INCLUDE/aw_global.hxx
MG_main.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
MG_main.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
MG_main.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
MG_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_main.o: $(ARBHOME)/INCLUDE/awt.hxx

MG_names.o: merge.hxx
MG_names.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_names.o: $(ARBHOME)/INCLUDE/arb_assert.h
MG_names.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_names.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_names.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
MG_names.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
MG_names.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_names.o: $(ARBHOME)/INCLUDE/aw_window.hxx

MG_preserves.o: merge.hxx
MG_preserves.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_preserves.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_preserves.o: $(ARBHOME)/INCLUDE/arb_assert.h
MG_preserves.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_preserves.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_preserves.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MG_preserves.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_preserves.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
MG_preserves.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_preserves.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_preserves.o: $(ARBHOME)/INCLUDE/awt.hxx
MG_preserves.o: $(ARBHOME)/INCLUDE/smartptr.h

MG_species.o: merge.hxx
MG_species.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_species.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_species.o: $(ARBHOME)/INCLUDE/adGene.h
MG_species.o: $(ARBHOME)/INCLUDE/arb_assert.h
MG_species.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_species.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_species.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MG_species.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_species.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
MG_species.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_species.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_species.o: $(ARBHOME)/INCLUDE/awt.hxx
MG_species.o: $(ARBHOME)/INCLUDE/awt_changekey.hxx
MG_species.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
MG_species.o: $(ARBHOME)/INCLUDE/db_scanner.hxx
MG_species.o: $(ARBHOME)/INCLUDE/GEN.hxx
MG_species.o: $(ARBHOME)/INCLUDE/inline.h

MG_trees.o: merge.hxx
MG_trees.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_trees.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_trees.o: $(ARBHOME)/INCLUDE/arb_assert.h
MG_trees.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_trees.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_trees.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_trees.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
MG_trees.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_trees.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_trees.o: $(ARBHOME)/INCLUDE/awt.hxx
MG_trees.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
