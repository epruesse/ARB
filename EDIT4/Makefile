# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx

CPP_OBJECTS = ED4_nds.o ED4_secedit.o \
	 	ED4_main.o ED4_mini_classes.o ED4_edit_string.o ED4_base.o ED4_manager.o ED4_terminal.o \
	 	ED4_root.o EDB_root_bact.o ED4_window.o ED4_members.o ED4_no_class.o ED4_list.o ED4_text_terminals.o \
		ED4_cursor.o ED4_tools.o ED4_search.o ED4_block.o edit_naligner.o

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

.cxx.o: 
	$(CPP) $(cflags) -c -o $@ $(@:.o=.cxx)  $(CPPINCLUDES)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)


# DO NOT DELETE THIS LINE -- make depend depends on it.


ED4_base.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ED4_base.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ED4_base.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ED4_base.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ED4_base.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ED4_base.c.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
ED4_base.c.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
ED4_base.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ED4_base.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ED4_base.c.o:  
ED4_base.c.o:  
ED4_base.c.o:  
ED4_base.c.o:  
ED4_base.c.o:  
ED4_base.c.o:  
ED4_base.c.o:  
ED4_base.c.o:   
ED4_base.c.o:  
ED4_base.c.o:  
ED4_base.c.o:  
ED4_base.c.o:  
ED4_base.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ED4_base.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ED4_base.c.o: ed4_awars.hxx ed4_class.hxx ed4_defs.hxx ed4_edit_string.hxx
ED4_base.c.o: ed4_search.hxx
ED4_block.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ED4_block.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ED4_block.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ED4_block.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ED4_block.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ED4_block.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ED4_block.c.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
ED4_block.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ED4_block.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ED4_block.c.o: $(ARBHOME)/INCLUDE/awtc_fast_aligner.hxx
ED4_block.c.o:  
ED4_block.c.o:  
ED4_block.c.o:  
ED4_block.c.o:  
ED4_block.c.o:  
ED4_block.c.o:  
ED4_block.c.o:  
ED4_block.c.o:  
ED4_block.c.o:  
ED4_block.c.o:  
ED4_block.c.o:  
ED4_block.c.o:  
ED4_block.c.o:  
ED4_block.c.o:  
ED4_block.c.o: 
ED4_block.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
ED4_block.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ED4_block.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ED4_block.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
ED4_block.c.o: ed4_awars.hxx ed4_block.hxx ed4_class.hxx ed4_defs.hxx
ED4_block.c.o: ed4_edit_string.hxx ed4_search.hxx ed4_tools.hxx
ED4_cursor.c.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
ED4_cursor.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ED4_cursor.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ED4_cursor.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ED4_cursor.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ED4_cursor.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ED4_cursor.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ED4_cursor.c.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
ED4_cursor.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ED4_cursor.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ED4_cursor.c.o:  
ED4_cursor.c.o:  
ED4_cursor.c.o:  
ED4_cursor.c.o:  
ED4_cursor.c.o:  
ED4_cursor.c.o:  
ED4_cursor.c.o:  
ED4_cursor.c.o:  
ED4_cursor.c.o:  
ED4_cursor.c.o:  
ED4_cursor.c.o:  
ED4_cursor.c.o:  
ED4_cursor.c.o:  
ED4_cursor.c.o:  
ED4_cursor.c.o: 
ED4_cursor.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
ED4_cursor.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ED4_cursor.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ED4_cursor.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
ED4_cursor.c.o: ed4_awars.hxx ed4_class.hxx ed4_defs.hxx ed4_edit_string.hxx
ED4_cursor.c.o: ed4_search.hxx ed4_secedit.hxx ed4_tools.hxx
ED4_edit_string.c.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
ED4_edit_string.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ED4_edit_string.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ED4_edit_string.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ED4_edit_string.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ED4_edit_string.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ED4_edit_string.c.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
ED4_edit_string.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ED4_edit_string.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ED4_edit_string.c.o: $(ARBHOME)/INCLUDE/awt_map_key.hxx
ED4_edit_string.c.o: $(ARBHOME)/INCLUDE/awtc_fast_aligner.hxx
ED4_edit_string.c.o:  
ED4_edit_string.c.o: 
ED4_edit_string.c.o: 
ED4_edit_string.c.o:  
ED4_edit_string.c.o:  
ED4_edit_string.c.o:  
ED4_edit_string.c.o:  
ED4_edit_string.c.o:  
ED4_edit_string.c.o:  
ED4_edit_string.c.o:  
ED4_edit_string.c.o:  
ED4_edit_string.c.o:  
ED4_edit_string.c.o:  
ED4_edit_string.c.o: 
ED4_edit_string.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ED4_edit_string.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ED4_edit_string.c.o: ed4_awars.hxx ed4_class.hxx ed4_defs.hxx
ED4_edit_string.c.o: ed4_edit_string.hxx ed4_search.hxx
ED4_list.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ED4_list.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ED4_list.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ED4_list.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ED4_list.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ED4_list.c.o:  
ED4_list.c.o:  
ED4_list.c.o:  
ED4_list.c.o:  
ED4_list.c.o:  
ED4_list.c.o:  
ED4_list.c.o:   
ED4_list.c.o:  
ED4_list.c.o:  
ED4_list.c.o:  
ED4_list.c.o: 
ED4_list.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ED4_list.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ED4_list.c.o: ed4_class.hxx ed4_defs.hxx ed4_search.hxx
ED4_main.c.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
ED4_main.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ED4_main.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ED4_main.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ED4_main.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ED4_main.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ED4_main.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ED4_main.c.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
ED4_main.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ED4_main.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ED4_main.c.o: $(ARBHOME)/INCLUDE/awt.hxx
ED4_main.c.o: $(ARBHOME)/INCLUDE/awt_map_key.hxx
ED4_main.c.o: $(ARBHOME)/INCLUDE/awt_seq_colors.hxx
ED4_main.c.o: $(ARBHOME)/INCLUDE/gde.hxx
ED4_main.c.o: $(ARBHOME)/INCLUDE/st_window.hxx
ED4_main.c.o:  
ED4_main.c.o:  
ED4_main.c.o:  
ED4_main.c.o:  
ED4_main.c.o:  
ED4_main.c.o:  
ED4_main.c.o:   
ED4_main.c.o:   
ED4_main.c.o:  
ED4_main.c.o:  
ED4_main.c.o:  
ED4_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ED4_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ED4_main.c.o: ed4_awars.hxx ed4_class.hxx ed4_defs.hxx ed4_edit_string.hxx
ED4_main.c.o: ed4_nds.hxx ed4_search.hxx edit_naligner.hxx
ED4_manager.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ED4_manager.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ED4_manager.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ED4_manager.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ED4_manager.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ED4_manager.c.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
ED4_manager.c.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
ED4_manager.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ED4_manager.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ED4_manager.c.o:  
ED4_manager.c.o:  
ED4_manager.c.o:  
ED4_manager.c.o:  
ED4_manager.c.o:  
ED4_manager.c.o:  
ED4_manager.c.o:  
ED4_manager.c.o:  
ED4_manager.c.o:  
ED4_manager.c.o:  
ED4_manager.c.o:  
ED4_manager.c.o:  
ED4_manager.c.o:  
ED4_manager.c.o:  
ED4_manager.c.o: 
ED4_manager.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
ED4_manager.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ED4_manager.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ED4_manager.c.o: ed4_awars.hxx ed4_class.hxx ed4_defs.hxx ed4_edit_string.hxx
ED4_manager.c.o: ed4_search.hxx ed4_tools.hxx
ED4_members.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ED4_members.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ED4_members.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ED4_members.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ED4_members.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ED4_members.c.o:  
ED4_members.c.o:  
ED4_members.c.o:  
ED4_members.c.o:  
ED4_members.c.o:  
ED4_members.c.o:  
ED4_members.c.o:  
ED4_members.c.o:  
ED4_members.c.o:  
ED4_members.c.o:  
ED4_members.c.o:  
ED4_members.c.o: 
ED4_members.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ED4_members.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ED4_members.c.o: ed4_class.hxx ed4_defs.hxx ed4_search.hxx
ED4_mini_classes.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ED4_mini_classes.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ED4_mini_classes.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ED4_mini_classes.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ED4_mini_classes.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ED4_mini_classes.c.o: $(ARBHOME)/INCLUDE/awt_iupac.hxx
ED4_mini_classes.c.o:  
ED4_mini_classes.c.o: 
ED4_mini_classes.c.o:  
ED4_mini_classes.c.o:  
ED4_mini_classes.c.o:  
ED4_mini_classes.c.o:  
ED4_mini_classes.c.o:  
ED4_mini_classes.c.o:  
ED4_mini_classes.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ED4_mini_classes.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ED4_mini_classes.c.o: ed4_awars.hxx ed4_class.hxx ed4_defs.hxx
ED4_mini_classes.c.o: ed4_edit_string.hxx ed4_search.hxx ed4_tools.hxx
ED4_nds.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ED4_nds.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ED4_nds.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ED4_nds.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ED4_nds.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ED4_nds.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ED4_nds.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ED4_nds.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ED4_nds.c.o:  
ED4_nds.c.o:  
ED4_nds.c.o:  
ED4_nds.c.o:  
ED4_nds.c.o:  
ED4_nds.c.o:  
ED4_nds.c.o:   
ED4_nds.c.o:  
ED4_nds.c.o:  
ED4_nds.c.o:  
ED4_nds.c.o: 
ED4_nds.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ED4_nds.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ED4_nds.c.o: ed4_awars.hxx ed4_class.hxx ed4_defs.hxx ed4_search.hxx
ED4_no_class.c.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
ED4_no_class.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ED4_no_class.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ED4_no_class.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ED4_no_class.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ED4_no_class.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ED4_no_class.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ED4_no_class.c.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
ED4_no_class.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ED4_no_class.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ED4_no_class.c.o: $(ARBHOME)/INCLUDE/awt.hxx
ED4_no_class.c.o: $(ARBHOME)/INCLUDE/awt_seq_colors.hxx
ED4_no_class.c.o: $(ARBHOME)/INCLUDE/awtc_rename.hxx
ED4_no_class.c.o: $(ARBHOME)/INCLUDE/st_window.hxx
ED4_no_class.c.o:  
ED4_no_class.c.o:  
ED4_no_class.c.o:  
ED4_no_class.c.o:  
ED4_no_class.c.o:  
ED4_no_class.c.o:  
ED4_no_class.c.o:  
ED4_no_class.c.o:  
ED4_no_class.c.o:  
ED4_no_class.c.o:  
ED4_no_class.c.o:  
ED4_no_class.c.o:  
ED4_no_class.c.o:  
ED4_no_class.c.o:  
ED4_no_class.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
ED4_no_class.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ED4_no_class.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ED4_no_class.c.o: ed4_awars.hxx ed4_class.hxx ed4_defs.hxx ed4_edit_string.hxx
ED4_no_class.c.o: ed4_nds.hxx ed4_search.hxx ed4_tools.hxx
ED4_root.c.o: ../NTREE/nt_date.h
ED4_root.c.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
ED4_root.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ED4_root.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ED4_root.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ED4_root.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ED4_root.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ED4_root.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ED4_root.c.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
ED4_root.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ED4_root.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ED4_root.c.o: $(ARBHOME)/INCLUDE/awt.hxx
ED4_root.c.o: $(ARBHOME)/INCLUDE/awt_map_key.hxx
ED4_root.c.o: $(ARBHOME)/INCLUDE/awt_seq_colors.hxx
ED4_root.c.o: $(ARBHOME)/INCLUDE/awtc_constructSequence.hxx
ED4_root.c.o: $(ARBHOME)/INCLUDE/awtc_fast_aligner.hxx
ED4_root.c.o: $(ARBHOME)/INCLUDE/gde.hxx
ED4_root.c.o: $(ARBHOME)/INCLUDE/st_window.hxx
ED4_root.c.o:  
ED4_root.c.o:  
ED4_root.c.o:  
ED4_root.c.o:  
ED4_root.c.o:  
ED4_root.c.o:   
ED4_root.c.o:  
ED4_root.c.o: 
ED4_root.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ED4_root.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ED4_root.c.o: ed4_awars.hxx ed4_block.hxx ed4_class.hxx ed4_defs.hxx
ED4_root.c.o: ed4_nds.hxx ed4_search.hxx ed4_secedit.hxx ed4_tools.hxx
ED4_root.c.o: edit_naligner.hxx
ED4_search.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ED4_search.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ED4_search.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ED4_search.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ED4_search.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ED4_search.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ED4_search.c.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
ED4_search.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ED4_search.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ED4_search.c.o: $(ARBHOME)/INCLUDE/awt.hxx
ED4_search.c.o: $(ARBHOME)/INCLUDE/awtc_fast_aligner.hxx
ED4_search.c.o:  
ED4_search.c.o:  
ED4_search.c.o:  
ED4_search.c.o:  
ED4_search.c.o:  
ED4_search.c.o:  
ED4_search.c.o:  
ED4_search.c.o:  
ED4_search.c.o:  
ED4_search.c.o:  
ED4_search.c.o:  
ED4_search.c.o:  
ED4_search.c.o:  
ED4_search.c.o:  
ED4_search.c.o:  
ED4_search.c.o:  
ED4_search.c.o: 
ED4_search.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
ED4_search.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ED4_search.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ED4_search.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
ED4_search.c.o: ed4_awars.hxx ed4_class.hxx ed4_defs.hxx ed4_search.hxx
ED4_search.c.o: ed4_tools.hxx
ED4_secedit.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ED4_secedit.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ED4_secedit.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ED4_secedit.c.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
ED4_secedit.c.o: $(ARBHOME)/INCLUDE/sec_graphic.hxx
ED4_secedit.c.o: $(ARBHOME)/INCLUDE/secedit.hxx
ED4_secedit.c.o:  
ED4_secedit.c.o:  
ED4_secedit.c.o:  
ED4_secedit.c.o:  
ED4_secedit.c.o:  
ED4_secedit.c.o:  
ED4_secedit.c.o:  
ED4_secedit.c.o:  
ED4_secedit.c.o:  
ED4_secedit.c.o:  
ED4_secedit.c.o:  
ED4_secedit.c.o:  
ED4_secedit.c.o:  
ED4_secedit.c.o:  
ED4_secedit.c.o:  
ED4_secedit.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
ED4_secedit.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ED4_secedit.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ED4_secedit.c.o: ed4_secedit.hxx
ED4_terminal.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ED4_terminal.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ED4_terminal.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ED4_terminal.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ED4_terminal.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ED4_terminal.c.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
ED4_terminal.c.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
ED4_terminal.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ED4_terminal.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ED4_terminal.c.o: $(ARBHOME)/INCLUDE/awt_seq_colors.hxx
ED4_terminal.c.o: $(ARBHOME)/INCLUDE/st_window.hxx
ED4_terminal.c.o:  
ED4_terminal.c.o:  
ED4_terminal.c.o:  
ED4_terminal.c.o:  
ED4_terminal.c.o:  
ED4_terminal.c.o:  
ED4_terminal.c.o:  
ED4_terminal.c.o:  
ED4_terminal.c.o:  
ED4_terminal.c.o:  
ED4_terminal.c.o:  
ED4_terminal.c.o:  
ED4_terminal.c.o: 
ED4_terminal.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ED4_terminal.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ED4_terminal.c.o: ed4_awars.hxx ed4_block.hxx ed4_class.hxx ed4_defs.hxx
ED4_terminal.c.o: ed4_edit_string.hxx ed4_nds.hxx ed4_search.hxx
ED4_text_terminals.c.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
ED4_text_terminals.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ED4_text_terminals.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ED4_text_terminals.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ED4_text_terminals.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ED4_text_terminals.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ED4_text_terminals.c.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
ED4_text_terminals.c.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
ED4_text_terminals.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ED4_text_terminals.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ED4_text_terminals.c.o: $(ARBHOME)/INCLUDE/awt_seq_colors.hxx
ED4_text_terminals.c.o: $(ARBHOME)/INCLUDE/st_window.hxx
ED4_text_terminals.c.o:  
ED4_text_terminals.c.o: 
ED4_text_terminals.c.o: 
ED4_text_terminals.c.o:  
ED4_text_terminals.c.o: 
ED4_text_terminals.c.o: 
ED4_text_terminals.c.o:  
ED4_text_terminals.c.o:  
ED4_text_terminals.c.o:  
ED4_text_terminals.c.o:  
ED4_text_terminals.c.o:  
ED4_text_terminals.c.o:  
ED4_text_terminals.c.o:  
ED4_text_terminals.c.o:  
ED4_text_terminals.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ED4_text_terminals.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ED4_text_terminals.c.o: ed4_awars.hxx ed4_block.hxx ed4_class.hxx ed4_defs.hxx
ED4_text_terminals.c.o: ed4_edit_string.hxx ed4_nds.hxx ed4_search.hxx
ED4_tools.c.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
ED4_tools.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ED4_tools.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ED4_tools.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ED4_tools.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ED4_tools.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
ED4_tools.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ED4_tools.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ED4_tools.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ED4_tools.c.o: $(ARBHOME)/INCLUDE/awt_iupac.hxx
ED4_tools.c.o: $(ARBHOME)/INCLUDE/awtc_fast_aligner.hxx
ED4_tools.c.o:  
ED4_tools.c.o:  
ED4_tools.c.o:  
ED4_tools.c.o:  
ED4_tools.c.o:  
ED4_tools.c.o:  
ED4_tools.c.o:  
ED4_tools.c.o:  
ED4_tools.c.o:  
ED4_tools.c.o:  
ED4_tools.c.o:  
ED4_tools.c.o: 
ED4_tools.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ED4_tools.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ED4_tools.c.o: ed4_awars.hxx ed4_class.hxx ed4_defs.hxx ed4_edit_string.hxx
ED4_tools.c.o: ed4_search.hxx ed4_tools.hxx
ED4_window.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ED4_window.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ED4_window.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
ED4_window.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
ED4_window.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
ED4_window.c.o:  
ED4_window.c.o:  
ED4_window.c.o:  
ED4_window.c.o:  
ED4_window.c.o:  
ED4_window.c.o:  
ED4_window.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ED4_window.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ED4_window.c.o: ed4_class.hxx ed4_defs.hxx ed4_search.hxx ed4_tools.hxx
EDB_root_bact.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
EDB_root_bact.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
EDB_root_bact.c.o: $(ARBHOME)/INCLUDE/arbdb.h
EDB_root_bact.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
EDB_root_bact.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
EDB_root_bact.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
EDB_root_bact.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
EDB_root_bact.c.o:  
EDB_root_bact.c.o:  
EDB_root_bact.c.o:  
EDB_root_bact.c.o:  
EDB_root_bact.c.o:  
EDB_root_bact.c.o:  
EDB_root_bact.c.o:  
EDB_root_bact.c.o:  
EDB_root_bact.c.o:  
EDB_root_bact.c.o:  
EDB_root_bact.c.o:  
EDB_root_bact.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
EDB_root_bact.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
EDB_root_bact.c.o: ed4_class.hxx ed4_defs.hxx ed4_search.hxx
edit_naligner.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
edit_naligner.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
edit_naligner.c.o: $(ARBHOME)/INCLUDE/arbdb.h
edit_naligner.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
edit_naligner.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
edit_naligner.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
edit_naligner.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
edit_naligner.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
edit_naligner.c.o:  
edit_naligner.c.o:  
edit_naligner.c.o:  
edit_naligner.c.o:  
edit_naligner.c.o:  
edit_naligner.c.o:  
edit_naligner.c.o:  
edit_naligner.c.o:  
edit_naligner.c.o:  
edit_naligner.c.o:  
edit_naligner.c.o:  
edit_naligner.c.o:  
edit_naligner.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
edit_naligner.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
