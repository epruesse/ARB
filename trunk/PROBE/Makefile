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

PT_new_design.o:  
PT_new_design.o:  
PT_new_design.o:  
PT_new_design.o:  
PT_new_design.o:  
PT_new_design.o:  
PT_new_design.o:  
PT_new_design.o:  
PT_new_design.o:  
PT_new_design.o:  
PT_new_design.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_new_design.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_new_design.o: $(ARBHOME)/INCLUDE/struct_man.h probe.h
PT_new_design.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_new_design.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_new_design.o: $(ARBHOME)/INCLUDE/ad_prot.h probe_tree.hxx
PT_new_design.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_new_design.o: $(ARBHOME)/INCLUDE/ad_t_prot.h pt_prototypes.h
PT_family.o:  
PT_family.o:  
PT_family.o:  
PT_family.o:  
PT_family.o:  
PT_family.o:  
PT_family.o:  
PT_family.o:  
PT_family.o:  
PT_family.o:  
PT_family.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_family.o: $(ARBHOME)/INCLUDE/struct_man.h
PT_family.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h probe.h
PT_family.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_family.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_family.o: $(ARBHOME)/INCLUDE/ad_prot.h probe_tree.hxx
PT_family.o: pt_prototypes.h $(ARBHOME)/INCLUDE/arbdbt.h
PT_family.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_prefixtree.o:  
PT_prefixtree.o:  
PT_prefixtree.o:  
PT_prefixtree.o:  
PT_prefixtree.o:  
PT_prefixtree.o:  
PT_prefixtree.o:  
PT_prefixtree.o:  
PT_prefixtree.o:  
PT_prefixtree.o:  
PT_prefixtree.o:  
PT_prefixtree.o: 
PT_prefixtree.o: $(ARBHOME)/INCLUDE/PT_server.h probe.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_prefixtree.o: $(ARBHOME)/INCLUDE/ad_prot.h probe_tree.hxx
PT_main.o:  
PT_main.o:  
PT_main.o:  
PT_main.o:  
PT_main.o:   
PT_main.o:  
PT_main.o:  
PT_main.o:  
PT_main.o:  
PT_main.o:  
PT_main.o:  
PT_main.o:  
PT_main.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_main.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h probe.h
PT_main.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_main.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_main.o: $(ARBHOME)/INCLUDE/ad_prot.h pt_prototypes.h
PT_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
PT_main.o: $(ARBHOME)/INCLUDE/servercntrl.h
PT_main.o: $(ARBHOME)/INCLUDE/server.h
PT_main.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
PT_main.o: $(ARBHOME)/INCLUDE/client.h
PT_main.o: $(ARBHOME)/INCLUDE/struct_man.h
PT_io.o:  
PT_io.o:  
PT_io.o:  
PT_io.o:  
PT_io.o:   
PT_io.o:  
PT_io.o:  
PT_io.o:  
PT_io.o:   
PT_io.o:  $(ARBHOME)/INCLUDE/PT_server.h
PT_io.o: probe.h $(ARBHOME)/INCLUDE/PT_com.h
PT_io.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_io.o: $(ARBHOME)/INCLUDE/ad_prot.h
PT_io.o: $(ARBHOME)/INCLUDE/arbdbt.h
PT_io.o: $(ARBHOME)/INCLUDE/ad_t_prot.h ./BI_helix.hxx
PT_etc.o:  
PT_etc.o:  
PT_etc.o:  
PT_etc.o:  
PT_etc.o:   
PT_etc.o:  
PT_etc.o:  
PT_etc.o:  
PT_etc.o:   
PT_etc.o:  $(ARBHOME)/INCLUDE/PT_server.h
PT_etc.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h probe.h
PT_etc.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_etc.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_etc.o: $(ARBHOME)/INCLUDE/ad_prot.h pt_prototypes.h
PT_etc.o: $(ARBHOME)/INCLUDE/struct_man.h
PT_secundaer.o:  
PT_secundaer.o:  
PT_secundaer.o:  
PT_secundaer.o:  
PT_secundaer.o:  
PT_secundaer.o:  
PT_secundaer.o:  
PT_secundaer.o:  
PT_secundaer.o:  
PT_secundaer.o:  
PT_secundaer.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_secundaer.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_secundaer.o: probe.h $(ARBHOME)/INCLUDE/PT_com.h
PT_secundaer.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_secundaer.o: $(ARBHOME)/INCLUDE/ad_prot.h pt_prototypes.h
PT_buildtree.o:  
PT_buildtree.o:  
PT_buildtree.o:  
PT_buildtree.o:  
PT_buildtree.o:  
PT_buildtree.o:  
PT_buildtree.o:  
PT_buildtree.o:  
PT_buildtree.o:  
PT_buildtree.o:  
PT_buildtree.o:  
PT_buildtree.o:  
PT_buildtree.o: $(ARBHOME)/INCLUDE/PT_server.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_buildtree.o: probe.h $(ARBHOME)/INCLUDE/PT_com.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_buildtree.o: $(ARBHOME)/INCLUDE/ad_prot.h probe_tree.hxx
PT_buildtree.o: pt_prototypes.h
PT_match.o:  
PT_match.o:  
PT_match.o:  
PT_match.o:  
PT_match.o:  
PT_match.o:  
PT_match.o:  
PT_match.o:  
PT_match.o:  
PT_match.o:  $(ARBHOME)/INCLUDE/PT_server.h
PT_match.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
PT_match.o: $(ARBHOME)/INCLUDE/struct_man.h probe.h
PT_match.o: $(ARBHOME)/INCLUDE/PT_com.h
PT_match.o: $(ARBHOME)/INCLUDE/arbdb.h
PT_match.o: $(ARBHOME)/INCLUDE/ad_prot.h probe_tree.hxx
PT_match.o: pt_prototypes.h $(ARBHOME)/INCLUDE/arbdbt.h
PT_match.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
