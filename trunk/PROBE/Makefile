.SUFFIXES: .o .cxx
OBJECTS = PT_new_design.o PT_family.o PT_prefixtree.o PT_main.o PT_io.o \
	PT_etc.o PT_secundaer.o \
	PT_buildtree.o PT_match.o 
AOBJECTS = BI_helix.o

$(MAIN): $(OBJECTS) $(AOBJECTS)
	$(AR) $(MAIN) $(OBJECTS) $(AOBJECTS)
.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES)  -I./../probe_bank/COM
.cxx.o: 
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES) -I./../probe_bank/COM
proto:
	rm -f pt_prototypes.h
	../MAKEBIN/aisc_mkpt -E $(OBJECTS:.o=.cxx) probe_tree.hxx >pt_prototypes.h
depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(OBJECTS:.o=.cxx)
# DO NOT DELETE THIS LINE -- make depend depends on it.


PT_buildtree.c.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_buildtree.c.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_buildtree.c.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_buildtree.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_buildtree.c.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_buildtree.c.o:  
PT_buildtree.c.o:  
PT_buildtree.c.o: 
PT_buildtree.c.o: 
PT_buildtree.c.o:  
PT_buildtree.c.o:  
PT_buildtree.c.o:  
PT_buildtree.c.o:  
PT_buildtree.c.o:  
PT_buildtree.c.o:  
PT_buildtree.c.o:  
PT_buildtree.c.o:  
PT_buildtree.c.o:  
PT_buildtree.c.o:  
PT_buildtree.c.o:  
PT_buildtree.c.o: 
PT_buildtree.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
PT_buildtree.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
PT_buildtree.c.o: probe.h probe_tree.hxx pt_prototypes.h
PT_etc.c.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_etc.c.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_etc.c.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_etc.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_etc.c.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_etc.c.o: $(ARBHOME)/INCLUDE/struct_man.h
PT_etc.c.o:  
PT_etc.c.o:  
PT_etc.c.o:  
PT_etc.c.o:  
PT_etc.c.o:  
PT_etc.c.o:  
PT_etc.c.o:   
PT_etc.c.o:   
PT_etc.c.o:  
PT_etc.c.o:  
PT_etc.c.o:  
PT_etc.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
PT_etc.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h probe.h
PT_etc.c.o: pt_prototypes.h
PT_family.c.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_family.c.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_family.c.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_family.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_family.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_family.c.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_family.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_family.c.o: $(ARBHOME)/INCLUDE/struct_man.h
PT_family.c.o:  
PT_family.c.o:  
PT_family.c.o:  
PT_family.c.o:  
PT_family.c.o:  
PT_family.c.o:  
PT_family.c.o:   
PT_family.c.o:  
PT_family.c.o:  
PT_family.c.o:  
PT_family.c.o:  
PT_family.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
PT_family.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
PT_family.c.o: probe.h probe_tree.hxx pt_prototypes.h
PT_io.c.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_io.c.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_io.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_io.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_io.c.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_io.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_io.c.o:  
PT_io.c.o:  
PT_io.c.o:  
PT_io.c.o:  
PT_io.c.o:  
PT_io.c.o:  
PT_io.c.o:   
PT_io.c.o:   
PT_io.c.o:  
PT_io.c.o:  
PT_io.c.o:  
PT_io.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
PT_io.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
PT_io.c.o: BI_helix.hxx probe.h
PT_main.c.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_main.c.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_main.c.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_main.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_main.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_main.c.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
PT_main.c.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_main.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_main.c.o: $(ARBHOME)/INCLUDE/client.h
PT_main.c.o: $(ARBHOME)/INCLUDE/server.h
PT_main.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
PT_main.c.o: $(ARBHOME)/INCLUDE/struct_man.h
PT_main.c.o:  
PT_main.c.o:  
PT_main.c.o:  
PT_main.c.o:  
PT_main.c.o:  
PT_main.c.o:  
PT_main.c.o:  
PT_main.c.o:  
PT_main.c.o:  
PT_main.c.o:   
PT_main.c.o:  
PT_main.c.o:  
PT_main.c.o:  
PT_main.c.o:   
PT_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
PT_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h probe.h
PT_main.c.o: pt_prototypes.h
PT_match.c.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_match.c.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_match.c.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_match.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_match.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_match.c.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_match.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_match.c.o: $(ARBHOME)/INCLUDE/struct_man.h
PT_match.c.o:  
PT_match.c.o:  
PT_match.c.o:  
PT_match.c.o:  
PT_match.c.o:  
PT_match.c.o:  
PT_match.c.o:   
PT_match.c.o:  
PT_match.c.o:  
PT_match.c.o:  
PT_match.c.o: 
PT_match.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
PT_match.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h probe.h
PT_match.c.o: probe_tree.hxx pt_prototypes.h
PT_new_design.c.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_new_design.c.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_new_design.c.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_new_design.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_new_design.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_new_design.c.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_new_design.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_new_design.c.o: $(ARBHOME)/INCLUDE/struct_man.h
PT_new_design.c.o:  
PT_new_design.c.o:  
PT_new_design.c.o:  
PT_new_design.c.o:  
PT_new_design.c.o:  
PT_new_design.c.o:  
PT_new_design.c.o:  
PT_new_design.c.o:  
PT_new_design.c.o:  
PT_new_design.c.o:  
PT_new_design.c.o:  
PT_new_design.c.o: 
PT_new_design.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
PT_new_design.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
PT_new_design.c.o: probe.h probe_tree.hxx pt_prototypes.h
PT_prefixtree.c.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_prefixtree.c.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_prefixtree.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_prefixtree.c.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_prefixtree.c.o:  
PT_prefixtree.c.o:  
PT_prefixtree.c.o:  
PT_prefixtree.c.o:  
PT_prefixtree.c.o:  
PT_prefixtree.c.o:  
PT_prefixtree.c.o:  
PT_prefixtree.c.o:  
PT_prefixtree.c.o:  
PT_prefixtree.c.o:  
PT_prefixtree.c.o:  
PT_prefixtree.c.o:  
PT_prefixtree.c.o:  
PT_prefixtree.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
PT_prefixtree.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
PT_prefixtree.c.o: probe.h probe_tree.hxx
PT_secundaer.c.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_secundaer.c.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_secundaer.c.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_secundaer.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_secundaer.c.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_secundaer.c.o:  
PT_secundaer.c.o:  
PT_secundaer.c.o:  
PT_secundaer.c.o:  
PT_secundaer.c.o:  
PT_secundaer.c.o:  
PT_secundaer.c.o:  
PT_secundaer.c.o:  
PT_secundaer.c.o:  
PT_secundaer.c.o:  
PT_secundaer.c.o:  
PT_secundaer.c.o: 
PT_secundaer.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
PT_secundaer.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
PT_secundaer.c.o: probe.h pt_prototypes.h
