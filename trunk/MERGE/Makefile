# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx
CPP_OBJECTS =   MG_checkfield.o MG_main.o MG_trees.o MG_alignment.o \
	MG_species.o MG_extendeds.o MG_names.o

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


MG_alignment.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_alignment.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_alignment.c.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_alignment.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_alignment.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_alignment.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_alignment.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_alignment.c.o: $(ARBHOME)/INCLUDE/awt.hxx
MG_alignment.c.o:  
MG_alignment.c.o:  
MG_alignment.c.o: 
MG_alignment.c.o: 
MG_alignment.c.o:  
MG_alignment.c.o:  
MG_alignment.c.o:  
MG_alignment.c.o:  
MG_alignment.c.o:  
MG_alignment.c.o:  
MG_alignment.c.o:  
MG_alignment.c.o:  
MG_alignment.c.o:  
MG_alignment.c.o:  
MG_alignment.c.o:  
MG_alignment.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MG_alignment.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MG_alignment.c.o: merge.hxx
MG_checkfield.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_checkfield.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_checkfield.c.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_checkfield.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_checkfield.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_checkfield.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_checkfield.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_checkfield.c.o: $(ARBHOME)/INCLUDE/awt.hxx
MG_checkfield.c.o:  
MG_checkfield.c.o:  
MG_checkfield.c.o:  
MG_checkfield.c.o:  
MG_checkfield.c.o:  
MG_checkfield.c.o:  
MG_checkfield.c.o:  
MG_checkfield.c.o: 
MG_checkfield.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MG_checkfield.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MG_checkfield.c.o: merge.hxx
MG_extendeds.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_extendeds.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_extendeds.c.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_extendeds.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_extendeds.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_extendeds.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_extendeds.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_extendeds.c.o: $(ARBHOME)/INCLUDE/awt.hxx
MG_extendeds.c.o:  
MG_extendeds.c.o:  
MG_extendeds.c.o:  
MG_extendeds.c.o:  
MG_extendeds.c.o:  
MG_extendeds.c.o:  
MG_extendeds.c.o:  
MG_extendeds.c.o:  
MG_extendeds.c.o:  
MG_extendeds.c.o:  
MG_extendeds.c.o: 
MG_extendeds.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MG_extendeds.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MG_extendeds.c.o: merge.hxx
MG_main.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_main.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_main.c.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_main.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_main.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MG_main.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_main.c.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
MG_main.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_main.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_main.c.o: $(ARBHOME)/INCLUDE/awt.hxx
MG_main.c.o: $(ARBHOME)/INCLUDE/awtc_rename.hxx
MG_main.c.o:  
MG_main.c.o:  
MG_main.c.o:  
MG_main.c.o:  
MG_main.c.o:  
MG_main.c.o:  
MG_main.c.o:   
MG_main.c.o:  
MG_main.c.o:  
MG_main.c.o:  
MG_main.c.o: 
MG_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MG_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MG_main.c.o: merge.hxx mg_merge.hxx
MG_names.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_names.c.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_names.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_names.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_names.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_names.c.o: $(ARBHOME)/INCLUDE/awtc_rename.hxx
MG_names.c.o:  
MG_names.c.o:  
MG_names.c.o:  
MG_names.c.o:  
MG_names.c.o:  
MG_names.c.o:  
MG_names.c.o:   
MG_names.c.o:  
MG_names.c.o:  
MG_names.c.o:  
MG_names.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MG_names.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MG_names.c.o: merge.hxx
MG_species.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_species.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_species.c.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_species.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_species.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MG_species.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_species.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_species.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_species.c.o: $(ARBHOME)/INCLUDE/awt.hxx
MG_species.c.o:  
MG_species.c.o:  
MG_species.c.o:  
MG_species.c.o:  
MG_species.c.o:  
MG_species.c.o:  
MG_species.c.o:  
MG_species.c.o:  
MG_species.c.o:  
MG_species.c.o:  
MG_species.c.o:  
MG_species.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MG_species.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MG_species.c.o: merge.hxx
MG_trees.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
MG_trees.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MG_trees.c.o: $(ARBHOME)/INCLUDE/arbdb.h
MG_trees.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
MG_trees.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MG_trees.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MG_trees.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MG_trees.c.o: $(ARBHOME)/INCLUDE/awt.hxx
MG_trees.c.o:  
MG_trees.c.o:  
MG_trees.c.o:  
MG_trees.c.o:  
MG_trees.c.o:  
MG_trees.c.o:  
MG_trees.c.o:   
MG_trees.c.o:  
MG_trees.c.o:  
MG_trees.c.o:  
MG_trees.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MG_trees.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MG_trees.c.o: merge.hxx
