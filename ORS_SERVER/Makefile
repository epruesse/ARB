# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx
OBJECTS = ORS_S_main.o ORS_S_userdb.o ORS_S_probedb.o ORS_S_lib.o ORS_S_cf.o ORS_lib.o

$(MAIN): ors_s_proto.hxx $(OBJECTS)
	$(AR) $(MAIN) $(OBJECTS) $(CRYPTLIB)
.c.o: 
	$(ACC) $(cflags) -c $<  $(AINCLUDES)
.cxx.o: 
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)
ors_s_proto.hxx: $(OBJECTS:.o=.cxx)
	rm -f ors_s_proto.hxx
	../MAKEBIN/aisc_mkpt -E $(OBJECTS:.o=.cxx) >ors_s_proto.hxx
proto: ors_s_proto.hxx

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(OBJECTS:.o=.cxx)

# DO NOT DELETE THIS LINE -- make depend depends on it.


ORS_S_cf.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_S_cf.c.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ORS_S_cf.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_S_cf.c.o: $(ARBHOME)/INCLUDE/client.h
ORS_S_cf.c.o: $(ARBHOME)/INCLUDE/ors_client.h
ORS_S_cf.c.o: $(ARBHOME)/INCLUDE/ors_prototypes.h
ORS_S_cf.c.o: $(ARBHOME)/INCLUDE/ors_server.h
ORS_S_cf.c.o: $(ARBHOME)/INCLUDE/server.h
ORS_S_cf.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
ORS_S_cf.c.o: $(ARBHOME)/INCLUDE/struct_man.h
ORS_S_cf.c.o:  
ORS_S_cf.c.o:  
ORS_S_cf.c.o:  
ORS_S_cf.c.o:  
ORS_S_cf.c.o:  
ORS_S_cf.c.o:  
ORS_S_cf.c.o:   
ORS_S_cf.c.o:  
ORS_S_cf.c.o:  
ORS_S_cf.c.o:  
ORS_S_cf.c.o: 
ORS_S_cf.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ORS_S_cf.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ORS_S_cf.c.o: ors_lib.h ors_s.h ors_s_common.hxx ors_s_proto.hxx
ORS_S_lib.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_S_lib.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ORS_S_lib.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_S_lib.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ORS_S_lib.c.o: $(ARBHOME)/INCLUDE/ors_server.h
ORS_S_lib.c.o:  
ORS_S_lib.c.o:  
ORS_S_lib.c.o:  
ORS_S_lib.c.o:  
ORS_S_lib.c.o:  
ORS_S_lib.c.o:  
ORS_S_lib.c.o:   
ORS_S_lib.c.o:  
ORS_S_lib.c.o:  
ORS_S_lib.c.o:  
ORS_S_lib.c.o:  
ORS_S_lib.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ORS_S_lib.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ORS_S_lib.c.o: ors_lib.h ors_s_common.hxx ors_s_proto.hxx
ORS_S_main.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_S_main.c.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ORS_S_main.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_S_main.c.o: $(ARBHOME)/INCLUDE/client.h
ORS_S_main.c.o: $(ARBHOME)/INCLUDE/ors_client.h
ORS_S_main.c.o: $(ARBHOME)/INCLUDE/ors_prototypes.h
ORS_S_main.c.o: $(ARBHOME)/INCLUDE/ors_server.h
ORS_S_main.c.o: $(ARBHOME)/INCLUDE/server.h
ORS_S_main.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
ORS_S_main.c.o: $(ARBHOME)/INCLUDE/struct_man.h
ORS_S_main.c.o:  
ORS_S_main.c.o:  
ORS_S_main.c.o:  
ORS_S_main.c.o:  
ORS_S_main.c.o:  
ORS_S_main.c.o:  
ORS_S_main.c.o:  
ORS_S_main.c.o:  
ORS_S_main.c.o:  
ORS_S_main.c.o:  
ORS_S_main.c.o:  
ORS_S_main.c.o:  
ORS_S_main.c.o:  
ORS_S_main.c.o:  
ORS_S_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ORS_S_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ORS_S_main.c.o: ors_lib.h ors_s.h ors_s_common.hxx ors_s_proto.hxx
ORS_S_probedb.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_S_probedb.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ORS_S_probedb.c.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ORS_S_probedb.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_S_probedb.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ORS_S_probedb.c.o: $(ARBHOME)/INCLUDE/ors_client.h
ORS_S_probedb.c.o: $(ARBHOME)/INCLUDE/ors_prototypes.h
ORS_S_probedb.c.o: $(ARBHOME)/INCLUDE/ors_server.h
ORS_S_probedb.c.o: $(ARBHOME)/INCLUDE/server.h
ORS_S_probedb.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
ORS_S_probedb.c.o: $(ARBHOME)/INCLUDE/struct_man.h
ORS_S_probedb.c.o:  
ORS_S_probedb.c.o:  
ORS_S_probedb.c.o:  
ORS_S_probedb.c.o:  
ORS_S_probedb.c.o:  
ORS_S_probedb.c.o:  
ORS_S_probedb.c.o:  
ORS_S_probedb.c.o:  
ORS_S_probedb.c.o:  
ORS_S_probedb.c.o:  
ORS_S_probedb.c.o:  
ORS_S_probedb.c.o:  
ORS_S_probedb.c.o:  
ORS_S_probedb.c.o:  
ORS_S_probedb.c.o: 
ORS_S_probedb.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/float.h
ORS_S_probedb.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ORS_S_probedb.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ORS_S_probedb.c.o: ors_lib.h ors_s.h ors_s_common.hxx ors_s_proto.hxx
ORS_S_userdb.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_S_userdb.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ORS_S_userdb.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_S_userdb.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ORS_S_userdb.c.o: $(ARBHOME)/INCLUDE/ors_server.h
ORS_S_userdb.c.o:  
ORS_S_userdb.c.o:  
ORS_S_userdb.c.o:  
ORS_S_userdb.c.o:  
ORS_S_userdb.c.o:  
ORS_S_userdb.c.o:  
ORS_S_userdb.c.o:  
ORS_S_userdb.c.o:  
ORS_S_userdb.c.o:  
ORS_S_userdb.c.o:  
ORS_S_userdb.c.o:  
ORS_S_userdb.c.o: 
ORS_S_userdb.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ORS_S_userdb.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ORS_S_userdb.c.o: ors_lib.h ors_s_common.hxx ors_s_proto.hxx
ORS_lib.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_lib.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_lib.c.o: $(ARBHOME)/INCLUDE/client.h
ORS_lib.c.o: $(ARBHOME)/INCLUDE/ors_client.h
ORS_lib.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
ORS_lib.c.o:  
ORS_lib.c.o:  
ORS_lib.c.o:  
ORS_lib.c.o:  
ORS_lib.c.o:  
ORS_lib.c.o:   
ORS_lib.c.o:  
ORS_lib.c.o:   
ORS_lib.c.o:   
ORS_lib.c.o:  
ORS_lib.c.o:  
ORS_lib.c.o:  
ORS_lib.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ORS_lib.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ORS_lib.c.o: ors_lib.h
