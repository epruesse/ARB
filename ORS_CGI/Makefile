# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx
CPP_OBJECTS =	ORS_C_main.o ORS_C_PT.o ORS_C_html.o ORS_C_java.o \
		ORS_C_user.o ORS_C_lib.o ORS_C_java_main.o ORS_C_scripts.o \
		ORS_C_probe.o ORS_lib.o ORS_C_rtok.o
$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)
.cxx.o: 
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)

proto:
	rm -f ors_c_proto.hxx
	../MAKEBIN/aisc_mkpt $(CPP_OBJECTS:.o=.cxx) | grep -v '::' >ors_c_proto.hxx
depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)
# DO NOT DELETE THIS LINE -- make depend depends on it.


ORS_C_PT.c.o: $(ARBHOME)/INCLUDE/PT_com.h
ORS_C_PT.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_PT.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_PT.c.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ORS_C_PT.c.o: $(ARBHOME)/INCLUDE/client.h
ORS_C_PT.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
ORS_C_PT.c.o:  
ORS_C_PT.c.o:  
ORS_C_PT.c.o:  
ORS_C_PT.c.o:  
ORS_C_PT.c.o:  
ORS_C_PT.c.o:  
ORS_C_PT.c.o:   
ORS_C_PT.c.o:   
ORS_C_PT.c.o:  
ORS_C_PT.c.o:  
ORS_C_PT.c.o:  
ORS_C_PT.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ORS_C_PT.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ORS_C_PT.c.o: ors_c.hxx ors_c_proto.hxx ors_lib.h
ORS_C_html.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_html.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_html.c.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ORS_C_html.c.o: $(ARBHOME)/INCLUDE/client.h
ORS_C_html.c.o: $(ARBHOME)/INCLUDE/ors_client.h
ORS_C_html.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
ORS_C_html.c.o:  
ORS_C_html.c.o:  
ORS_C_html.c.o:  
ORS_C_html.c.o:  
ORS_C_html.c.o:  
ORS_C_html.c.o:  
ORS_C_html.c.o:  
ORS_C_html.c.o:  
ORS_C_html.c.o:  
ORS_C_html.c.o:  
ORS_C_html.c.o:  
ORS_C_html.c.o: 
ORS_C_html.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ORS_C_html.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ORS_C_html.c.o: ors_c.hxx ors_c_java.hxx ors_c_proto.hxx ors_lib.h
ORS_C_java.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_java.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ORS_C_java.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_java.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ORS_C_java.c.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ORS_C_java.c.o: $(ARBHOME)/INCLUDE/client.h
ORS_C_java.c.o: $(ARBHOME)/INCLUDE/ors_client.h
ORS_C_java.c.o:  
ORS_C_java.c.o:  
ORS_C_java.c.o:  
ORS_C_java.c.o:  
ORS_C_java.c.o:  
ORS_C_java.c.o:  
ORS_C_java.c.o:  
ORS_C_java.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ORS_C_java.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ORS_C_java.c.o: ors_c.hxx ors_c_java.hxx ors_c_proto.hxx
ORS_C_java_main.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_java_main.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ORS_C_java_main.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_java_main.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ORS_C_java_main.c.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ORS_C_java_main.c.o: $(ARBHOME)/INCLUDE/ors_client.h
ORS_C_java_main.c.o:  
ORS_C_java_main.c.o:  
ORS_C_java_main.c.o: 
ORS_C_java_main.c.o: 
ORS_C_java_main.c.o: 
ORS_C_java_main.c.o:  
ORS_C_java_main.c.o:  
ORS_C_java_main.c.o:  
ORS_C_java_main.c.o:  
ORS_C_java_main.c.o:  
ORS_C_java_main.c.o:  
ORS_C_java_main.c.o:  
ORS_C_java_main.c.o:  
ORS_C_java_main.c.o:  
ORS_C_java_main.c.o:  
ORS_C_java_main.c.o:  
ORS_C_java_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ORS_C_java_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ORS_C_java_main.c.o: ors_c.hxx ors_c_java.hxx ors_c_proto.hxx ors_lib.h
ORS_C_lib.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_lib.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_lib.c.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ORS_C_lib.c.o: $(ARBHOME)/INCLUDE/client.h
ORS_C_lib.c.o: $(ARBHOME)/INCLUDE/ors_client.h
ORS_C_lib.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
ORS_C_lib.c.o:  
ORS_C_lib.c.o:  
ORS_C_lib.c.o:  
ORS_C_lib.c.o:  
ORS_C_lib.c.o:  
ORS_C_lib.c.o:  
ORS_C_lib.c.o:  
ORS_C_lib.c.o:  
ORS_C_lib.c.o:  
ORS_C_lib.c.o:  
ORS_C_lib.c.o:  
ORS_C_lib.c.o:  
ORS_C_lib.c.o:  
ORS_C_lib.c.o:  
ORS_C_lib.c.o:  
ORS_C_lib.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ORS_C_lib.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ORS_C_lib.c.o: ors_c.hxx ors_c_java.hxx ors_c_proto.hxx ors_lib.h
ORS_C_main.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_main.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_main.c.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ORS_C_main.c.o: $(ARBHOME)/INCLUDE/client.h
ORS_C_main.c.o: $(ARBHOME)/INCLUDE/ors_client.h
ORS_C_main.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
ORS_C_main.c.o:  
ORS_C_main.c.o:  
ORS_C_main.c.o:  
ORS_C_main.c.o:  
ORS_C_main.c.o:  
ORS_C_main.c.o:  
ORS_C_main.c.o:  
ORS_C_main.c.o:  
ORS_C_main.c.o:  
ORS_C_main.c.o:  
ORS_C_main.c.o:  
ORS_C_main.c.o:  
ORS_C_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ORS_C_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ORS_C_main.c.o: ors_c.hxx ors_c_proto.hxx ors_lib.h
ORS_C_probe.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_probe.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_probe.c.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ORS_C_probe.c.o: $(ARBHOME)/INCLUDE/client.h
ORS_C_probe.c.o: $(ARBHOME)/INCLUDE/ors_client.h
ORS_C_probe.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
ORS_C_probe.c.o:  
ORS_C_probe.c.o:  
ORS_C_probe.c.o:  
ORS_C_probe.c.o:  
ORS_C_probe.c.o:  
ORS_C_probe.c.o:  
ORS_C_probe.c.o:  
ORS_C_probe.c.o:  
ORS_C_probe.c.o:  
ORS_C_probe.c.o:  
ORS_C_probe.c.o:  
ORS_C_probe.c.o:  
ORS_C_probe.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ORS_C_probe.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ORS_C_probe.c.o: ors_c.hxx ors_c_proto.hxx ors_lib.h
ORS_C_rtok.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_rtok.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_rtok.c.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ORS_C_rtok.c.o: $(ARBHOME)/INCLUDE/client.h
ORS_C_rtok.c.o: $(ARBHOME)/INCLUDE/ors_client.h
ORS_C_rtok.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
ORS_C_rtok.c.o:  
ORS_C_rtok.c.o:  
ORS_C_rtok.c.o:  
ORS_C_rtok.c.o:  
ORS_C_rtok.c.o:  
ORS_C_rtok.c.o:  
ORS_C_rtok.c.o:  
ORS_C_rtok.c.o:  
ORS_C_rtok.c.o:  
ORS_C_rtok.c.o:  
ORS_C_rtok.c.o:  
ORS_C_rtok.c.o: 
ORS_C_rtok.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ORS_C_rtok.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ORS_C_rtok.c.o: ors_c.hxx ors_c_java.hxx ors_c_proto.hxx ors_lib.h
ORS_C_scripts.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_scripts.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_scripts.c.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ORS_C_scripts.c.o: $(ARBHOME)/INCLUDE/ors_client.h
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o: 
ORS_C_scripts.c.o: 
ORS_C_scripts.c.o: 
ORS_C_scripts.c.o: 
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o: 
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o:  
ORS_C_scripts.c.o: 
ORS_C_scripts.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/limits.h
ORS_C_scripts.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ORS_C_scripts.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ORS_C_scripts.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/syslimits.h
ORS_C_scripts.c.o: ors_c.hxx ors_c_java.hxx ors_c_proto.hxx ors_lib.h
ORS_C_user.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ORS_C_user.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ORS_C_user.c.o: $(ARBHOME)/INCLUDE/cat_tree.hxx
ORS_C_user.c.o: $(ARBHOME)/INCLUDE/client.h
ORS_C_user.c.o: $(ARBHOME)/INCLUDE/ors_client.h
ORS_C_user.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
ORS_C_user.c.o:  
ORS_C_user.c.o:  
ORS_C_user.c.o:  
ORS_C_user.c.o:  
ORS_C_user.c.o:  
ORS_C_user.c.o:  
ORS_C_user.c.o:  
ORS_C_user.c.o:  
ORS_C_user.c.o:  
ORS_C_user.c.o:  
ORS_C_user.c.o:  
ORS_C_user.c.o:  
ORS_C_user.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ORS_C_user.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ORS_C_user.c.o: ors_c.hxx ors_c_java.hxx ors_c_proto.hxx
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
ORS_lib.c.o:  
ORS_lib.c.o:  
ORS_lib.c.o:   
ORS_lib.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ORS_lib.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ORS_lib.c.o: ors_lib.h
