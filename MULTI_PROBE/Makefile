# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx


CPP_OBJECTS =  MP_main.o MP_Window.o MP_noclass.o MP_sonde.o MP_sondentopf.o MP_mo_liste.o MP_probe.o \
		MP_GenerationDuplicates.o MP_Generation.o MP_probe_combi_statistic.o MP_probe_tabs.o \
		MP_permute.o


$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

.cxx.o: 
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

andy:
	CC -o mpw $(CPP_OBJECTS)

clean:
	rm *.o
	rm *.a
	rm *.bak
	rm *~
	rm *#
	rm *\%

#.IGNORE:
#	clean

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)
# DO NOT DELETE THIS LINE -- make depend depends on it.


MP_Generation.c.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_Generation.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_Generation.c.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_Generation.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_Generation.c.o: $(ARBHOME)/INCLUDE/client.h
MP_Generation.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
MP_Generation.c.o:  
MP_Generation.c.o:  
MP_Generation.c.o:  
MP_Generation.c.o:  
MP_Generation.c.o:  
MP_Generation.c.o:  
MP_Generation.c.o:  
MP_Generation.c.o:  
MP_Generation.c.o: 
MP_Generation.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
MP_Generation.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MP_Generation.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MP_Generation.c.o: MP_externs.hxx MP_probe.hxx MultiProbe.hxx SoTl.hxx
MP_Generation.c.o: mpdefs.h
MP_GenerationDuplicates.c.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_GenerationDuplicates.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_GenerationDuplicates.c.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_GenerationDuplicates.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_GenerationDuplicates.c.o: $(ARBHOME)/INCLUDE/client.h
MP_GenerationDuplicates.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
MP_GenerationDuplicates.c.o: 
MP_GenerationDuplicates.c.o: 
MP_GenerationDuplicates.c.o: 
MP_GenerationDuplicates.c.o: 
MP_GenerationDuplicates.c.o:  
MP_GenerationDuplicates.c.o:  
MP_GenerationDuplicates.c.o:  
MP_GenerationDuplicates.c.o:  
MP_GenerationDuplicates.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MP_GenerationDuplicates.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MP_GenerationDuplicates.c.o: MP_externs.hxx MP_probe.hxx MultiProbe.hxx
MP_GenerationDuplicates.c.o: SoTl.hxx mpdefs.h
MP_Window.c.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_Window.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_Window.c.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_Window.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_Window.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MP_Window.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MP_Window.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MP_Window.c.o: $(ARBHOME)/INCLUDE/awt.hxx
MP_Window.c.o: $(ARBHOME)/INCLUDE/client.h
MP_Window.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
MP_Window.c.o:  
MP_Window.c.o:  
MP_Window.c.o:  
MP_Window.c.o:  
MP_Window.c.o:  
MP_Window.c.o:  
MP_Window.c.o:   
MP_Window.c.o:  
MP_Window.c.o:  
MP_Window.c.o:  
MP_Window.c.o: 
MP_Window.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MP_Window.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MP_Window.c.o: MP_externs.hxx MP_probe.hxx MultiProbe.hxx SoTl.hxx mpdefs.h
MP_main.c.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_main.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_main.c.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_main.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_main.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MP_main.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MP_main.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MP_main.c.o: $(ARBHOME)/INCLUDE/client.h
MP_main.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
MP_main.c.o:  
MP_main.c.o:  
MP_main.c.o:  
MP_main.c.o:  
MP_main.c.o:  
MP_main.c.o:   
MP_main.c.o:  
MP_main.c.o:  
MP_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
MP_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MP_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MP_main.c.o: MP_externs.hxx MP_probe.hxx MultiProbe.hxx SoTl.hxx mpdefs.h
MP_mo_liste.c.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_mo_liste.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_mo_liste.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MP_mo_liste.c.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_mo_liste.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
MP_mo_liste.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_mo_liste.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MP_mo_liste.c.o: $(ARBHOME)/INCLUDE/client.h
MP_mo_liste.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
MP_mo_liste.c.o:  
MP_mo_liste.c.o:  
MP_mo_liste.c.o:  
MP_mo_liste.c.o:  
MP_mo_liste.c.o:  
MP_mo_liste.c.o:  
MP_mo_liste.c.o:  
MP_mo_liste.c.o:  
MP_mo_liste.c.o:  
MP_mo_liste.c.o:  
MP_mo_liste.c.o:  
MP_mo_liste.c.o: 
MP_mo_liste.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MP_mo_liste.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MP_mo_liste.c.o: MP_externs.hxx MP_probe.hxx MultiProbe.hxx SoTl.hxx mpdefs.h
MP_noclass.c.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_noclass.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_noclass.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MP_noclass.c.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_noclass.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
MP_noclass.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_noclass.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MP_noclass.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MP_noclass.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MP_noclass.c.o: $(ARBHOME)/INCLUDE/awt.hxx
MP_noclass.c.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
MP_noclass.c.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
MP_noclass.c.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
MP_noclass.c.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
MP_noclass.c.o: $(ARBHOME)/INCLUDE/awt_tree_cb.hxx
MP_noclass.c.o: $(ARBHOME)/INCLUDE/client.h
MP_noclass.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
MP_noclass.c.o:  
MP_noclass.c.o:  
MP_noclass.c.o:  
MP_noclass.c.o:  
MP_noclass.c.o:  
MP_noclass.c.o:  
MP_noclass.c.o:  
MP_noclass.c.o:  
MP_noclass.c.o:  
MP_noclass.c.o:   
MP_noclass.c.o:  
MP_noclass.c.o:  
MP_noclass.c.o:  
MP_noclass.c.o: 
MP_noclass.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
MP_noclass.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MP_noclass.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MP_noclass.c.o: MP_externs.hxx MP_probe.hxx MultiProbe.hxx SoTl.hxx mpdefs.h
MP_permute.c.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_permute.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_permute.c.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_permute.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_permute.c.o: $(ARBHOME)/INCLUDE/client.h
MP_permute.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
MP_permute.c.o:  
MP_permute.c.o:  
MP_permute.c.o:  
MP_permute.c.o:  
MP_permute.c.o:  
MP_permute.c.o:  
MP_permute.c.o:  
MP_permute.c.o:  
MP_permute.c.o:   
MP_permute.c.o:  
MP_permute.c.o:  
MP_permute.c.o:  
MP_permute.c.o:  
MP_permute.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
MP_permute.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MP_permute.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MP_permute.c.o: MP_externs.hxx MP_probe.hxx MultiProbe.hxx SoTl.hxx mpdefs.h
MP_probe.c.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_probe.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_probe.c.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_probe.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_probe.c.o: $(ARBHOME)/INCLUDE/aw_device.hxx
MP_probe.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MP_probe.c.o: $(ARBHOME)/INCLUDE/aw_window.hxx
MP_probe.c.o: $(ARBHOME)/INCLUDE/client.h
MP_probe.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
MP_probe.c.o:  
MP_probe.c.o:  
MP_probe.c.o:  
MP_probe.c.o:  
MP_probe.c.o:  
MP_probe.c.o:  
MP_probe.c.o:  
MP_probe.c.o:   
MP_probe.c.o:  
MP_probe.c.o:  
MP_probe.c.o:  
MP_probe.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MP_probe.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MP_probe.c.o: MP_externs.hxx MP_probe.hxx MultiProbe.hxx SoTl.hxx mpdefs.h
MP_probe_combi_statistic.c.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_probe_combi_statistic.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_probe_combi_statistic.c.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_probe_combi_statistic.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_probe_combi_statistic.c.o: $(ARBHOME)/INCLUDE/client.h
MP_probe_combi_statistic.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
MP_probe_combi_statistic.c.o: 
MP_probe_combi_statistic.c.o: 
MP_probe_combi_statistic.c.o: 
MP_probe_combi_statistic.c.o: 
MP_probe_combi_statistic.c.o: 
MP_probe_combi_statistic.c.o: 
MP_probe_combi_statistic.c.o: 
MP_probe_combi_statistic.c.o: 
MP_probe_combi_statistic.c.o: 
MP_probe_combi_statistic.c.o:  
MP_probe_combi_statistic.c.o:  
MP_probe_combi_statistic.c.o:  
MP_probe_combi_statistic.c.o:  
MP_probe_combi_statistic.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
MP_probe_combi_statistic.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MP_probe_combi_statistic.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MP_probe_combi_statistic.c.o: MP_externs.hxx MP_probe.hxx MultiProbe.hxx
MP_probe_combi_statistic.c.o: SoTl.hxx mpdefs.h
MP_probe_tabs.c.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_probe_tabs.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_probe_tabs.c.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_probe_tabs.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_probe_tabs.c.o: $(ARBHOME)/INCLUDE/client.h
MP_probe_tabs.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
MP_probe_tabs.c.o:  
MP_probe_tabs.c.o:  
MP_probe_tabs.c.o:  
MP_probe_tabs.c.o:  
MP_probe_tabs.c.o:  
MP_probe_tabs.c.o:  
MP_probe_tabs.c.o:  
MP_probe_tabs.c.o:  
MP_probe_tabs.c.o: 
MP_probe_tabs.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
MP_probe_tabs.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MP_probe_tabs.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MP_probe_tabs.c.o: MP_externs.hxx MP_probe.hxx MultiProbe.hxx SoTl.hxx
MP_probe_tabs.c.o: mpdefs.h
MP_sonde.c.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_sonde.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_sonde.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MP_sonde.c.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_sonde.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
MP_sonde.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_sonde.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MP_sonde.c.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
MP_sonde.c.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
MP_sonde.c.o: $(ARBHOME)/INCLUDE/client.h
MP_sonde.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
MP_sonde.c.o:  
MP_sonde.c.o:  
MP_sonde.c.o:  
MP_sonde.c.o:  
MP_sonde.c.o:  
MP_sonde.c.o:  
MP_sonde.c.o:  
MP_sonde.c.o:  
MP_sonde.c.o:  
MP_sonde.c.o:   
MP_sonde.c.o:  
MP_sonde.c.o:  
MP_sonde.c.o:  
MP_sonde.c.o: 
MP_sonde.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
MP_sonde.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MP_sonde.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MP_sonde.c.o: MP_externs.hxx MP_probe.hxx MultiProbe.hxx SoTl.hxx mpdefs.h
MP_sondentopf.c.o: $(ARBHOME)/INCLUDE/PT_com.h
MP_sondentopf.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
MP_sondentopf.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
MP_sondentopf.c.o: $(ARBHOME)/INCLUDE/arbdb.h
MP_sondentopf.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
MP_sondentopf.c.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
MP_sondentopf.c.o: $(ARBHOME)/INCLUDE/aw_root.hxx
MP_sondentopf.c.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
MP_sondentopf.c.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
MP_sondentopf.c.o: $(ARBHOME)/INCLUDE/client.h
MP_sondentopf.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
MP_sondentopf.c.o:  
MP_sondentopf.c.o:  
MP_sondentopf.c.o:  
MP_sondentopf.c.o:  
MP_sondentopf.c.o:  
MP_sondentopf.c.o:  
MP_sondentopf.c.o:  
MP_sondentopf.c.o:  
MP_sondentopf.c.o:  
MP_sondentopf.c.o:  
MP_sondentopf.c.o:  
MP_sondentopf.c.o:  
MP_sondentopf.c.o:  
MP_sondentopf.c.o:  
MP_sondentopf.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
MP_sondentopf.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
MP_sondentopf.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
MP_sondentopf.c.o: MP_externs.hxx MP_probe.hxx MultiProbe.hxx SoTl.hxx
MP_sondentopf.c.o: mpdefs.h
