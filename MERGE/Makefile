# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx
CPP_OBJECTS =   MG_checkfield.o MG_main.o MG_trees.o MG_alignment.o \
	MG_species.o MG_extendeds.o MG_names.o MG_gene_species.o MG_configs.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)
.cxx.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)
NT_cb.hxx: NT_cb.cxx NT_extern.cxx
	rm -f $@
	../MAKEBIN/aisc_mkpt -C NT_cb.cxx NT_extern.cxx >$@
depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)
# DO NOT DELETE THIS LINE -- make depend depends on it.

MG_checkfield.o:  $(ARBHOME)/INCLUDE/arbdb.h
MG_checkfield.o: $(ARBHOME)/INCLUDE/arb_assert.h
MG_checkfield.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_checkfield.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_checkfield.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_checkfield.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_checkfield.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_checkfield.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_checkfield.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MG_checkfield.o: $(ARBHOME)/INCLUDE/awt.hxx merge.hxx
MG_main.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
MG_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_main.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
MG_main.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MG_main.o: $(ARBHOME)/INCLUDE/awt.hxx
MG_main.o: $(ARBHOME)/INCLUDE/awtc_rename.hxx merge.hxx mg_merge.hxx
MG_trees.o:  $(ARBHOME)/INCLUDE/arbdb.h
MG_trees.o: $(ARBHOME)/INCLUDE/arb_assert.h
MG_trees.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_trees.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_trees.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_trees.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_trees.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_trees.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_trees.o: $(ARBHOME)/INCLUDE/awt.hxx merge.hxx
MG_alignment.o:  $(ARBHOME)/INCLUDE/arbdb.h
MG_alignment.o: $(ARBHOME)/INCLUDE/arb_assert.h
MG_alignment.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_alignment.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_alignment.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_alignment.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_alignment.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_alignment.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_alignment.o: $(ARBHOME)/INCLUDE/awt.hxx merge.hxx
MG_species.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_species.o: $(ARBHOME)/INCLUDE/arb_assert.h
MG_species.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_species.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_species.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_species.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_species.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_species.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_species.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MG_species.o: $(ARBHOME)/INCLUDE/awt.hxx merge.hxx
MG_extendeds.o:  $(ARBHOME)/INCLUDE/arbdb.h
MG_extendeds.o: $(ARBHOME)/INCLUDE/arb_assert.h
MG_extendeds.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_extendeds.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_extendeds.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_extendeds.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_extendeds.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_extendeds.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_extendeds.o: $(ARBHOME)/INCLUDE/awt.hxx
MG_extendeds.o: $(ARBHOME)/INCLUDE/aw_awars.hxx merge.hxx
MG_names.o:  $(ARBHOME)/INCLUDE/arbdb.h
MG_names.o: $(ARBHOME)/INCLUDE/arb_assert.h
MG_names.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_names.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_names.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_names.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_names.o: $(ARBHOME)/INCLUDE/awtc_rename.hxx merge.hxx
MG_gene_species.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_gene_species.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MG_gene_species.o: $(ARBHOME)/INCLUDE/awt.hxx
MG_gene_species.o: $(ARBHOME)/INCLUDE/awt_config_manager.hxx
MG_gene_species.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_gene_species.o: $(ARBHOME)/INCLUDE/arb_assert.h
MG_gene_species.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_gene_species.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_gene_species.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_gene_species.o: $(ARBHOME)/INCLUDE/adGene.h
MG_gene_species.o: $(ARBHOME)/INCLUDE/inline.h merge.hxx
MG_configs.o:  $(ARBHOME)/INCLUDE/arbdb.h
MG_configs.o: $(ARBHOME)/INCLUDE/arb_assert.h
MG_configs.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_configs.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_configs.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_configs.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_configs.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_configs.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_configs.o: $(ARBHOME)/INCLUDE/awt.hxx merge.hxx
