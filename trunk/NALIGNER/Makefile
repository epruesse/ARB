# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .cxx

#CFLAGS = -g
#CFLAGS = -O2
CFLAGS = $(cflags)

# CPP_OBJECTS = ali_prealigner.o ali_aligner.o ali_postree.o \
# 				  ali_arbdb.o ali_sequence.o ali_solution.o \
# 				  ali_pt.o ali_global.o ali_profile.o ali_pathmap.o \
# 				  ali_main.o BI_helix.o

CPP_OBJECTS = ali_prealigner.o ali_aligner.o \
				  ali_arbdb.o ali_sequence.o ali_solution.o \
				  ali_pt.o ali_global.o ali_profile.o ali_pathmap.o \
				  ali_main.o BI_helix.o


$(MAIN): $(CPP_OBJECTS) 
	$(CPP) -o $(MAIN:.a=.com) $(CPP_OBJECTS) $(CPPINCLUDES) $(LIBPATH) ../PROBE_COM/client.a ../SERVERCNTRL/SERVERCNTRL.a -lARBDB $(SYSLIBS) $(CCPLIBS)
	touch $(MAIN);

.cxx.o: 
	$(CPP) $(CFLAGS) -c -o $@ $< $(CPPINCLUDES)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)

# DO NOT DELETE THIS LINE -- make depend depends on it.


BI_helix.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
BI_helix.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
BI_helix.c.o: $(ARBHOME)/INCLUDE/arbdb.h
BI_helix.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
BI_helix.c.o:  
BI_helix.c.o:  
BI_helix.c.o:  
BI_helix.c.o:  
BI_helix.c.o:  
BI_helix.c.o:  
BI_helix.c.o:  
BI_helix.c.o:   
BI_helix.c.o:  
BI_helix.c.o:  
BI_helix.c.o:  
BI_helix.c.o: 
BI_helix.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
BI_helix.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
BI_helix.c.o: BI_helix.hxx
ali_aligner.c.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_aligner.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_aligner.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_aligner.c.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ali_aligner.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_aligner.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_aligner.c.o: $(ARBHOME)/INCLUDE/client.h
ali_aligner.c.o: $(ARBHOME)/INCLUDE/server.h
ali_aligner.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
ali_aligner.c.o:  
ali_aligner.c.o:  
ali_aligner.c.o: 
ali_aligner.c.o: 
ali_aligner.c.o:  
ali_aligner.c.o:  
ali_aligner.c.o:  
ali_aligner.c.o:  
ali_aligner.c.o:  
ali_aligner.c.o:  
ali_aligner.c.o:  
ali_aligner.c.o:  
ali_aligner.c.o:  
ali_aligner.c.o:  
ali_aligner.c.o:  
ali_aligner.c.o:  
ali_aligner.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ali_aligner.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ali_aligner.c.o: ali_aligner.hxx ali_arbdb.hxx ali_misc.hxx
ali_aligner.c.o: ali_other_stuff.hxx ali_pathmap.hxx ali_profile.hxx
ali_aligner.c.o: ali_pt.hxx ali_sequence.hxx ali_solution.hxx ali_tarray.hxx
ali_aligner.c.o: ali_tlist.hxx ali_tstack.hxx
ali_arbdb.c.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_arbdb.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_arbdb.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_arbdb.c.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ali_arbdb.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_arbdb.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_arbdb.c.o: $(ARBHOME)/INCLUDE/server.h
ali_arbdb.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
ali_arbdb.c.o:  
ali_arbdb.c.o:  
ali_arbdb.c.o:  
ali_arbdb.c.o:  
ali_arbdb.c.o:  
ali_arbdb.c.o:  
ali_arbdb.c.o:  
ali_arbdb.c.o:  
ali_arbdb.c.o:  
ali_arbdb.c.o:  
ali_arbdb.c.o:  
ali_arbdb.c.o:  
ali_arbdb.c.o:  
ali_arbdb.c.o:  
ali_arbdb.c.o:  
ali_arbdb.c.o: 
ali_arbdb.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ali_arbdb.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ali_arbdb.c.o: ali_arbdb.hxx ali_misc.hxx ali_other_stuff.hxx ali_sequence.hxx
ali_global.c.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_global.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_global.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_global.c.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ali_global.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_global.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_global.c.o: $(ARBHOME)/INCLUDE/client.h
ali_global.c.o: $(ARBHOME)/INCLUDE/server.h
ali_global.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
ali_global.c.o:  
ali_global.c.o:  
ali_global.c.o:  
ali_global.c.o:  
ali_global.c.o:  
ali_global.c.o:  
ali_global.c.o:  
ali_global.c.o:  
ali_global.c.o:  
ali_global.c.o:  
ali_global.c.o:  
ali_global.c.o:  
ali_global.c.o:  
ali_global.c.o:  
ali_global.c.o:  
ali_global.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ali_global.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ali_global.c.o: ali_arbdb.hxx ali_global.hxx ali_misc.hxx ali_other_stuff.hxx
ali_global.c.o: ali_pathmap.hxx ali_prealigner.hxx ali_profile.hxx ali_pt.hxx
ali_global.c.o: ali_sequence.hxx ali_solution.hxx ali_tarray.hxx ali_tlist.hxx
ali_global.c.o: ali_tstack.hxx
ali_main.c.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_main.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_main.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_main.c.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ali_main.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_main.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_main.c.o: $(ARBHOME)/INCLUDE/client.h
ali_main.c.o: $(ARBHOME)/INCLUDE/server.h
ali_main.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
ali_main.c.o:  
ali_main.c.o:  
ali_main.c.o:  
ali_main.c.o:  
ali_main.c.o:  
ali_main.c.o:  
ali_main.c.o:  
ali_main.c.o:  
ali_main.c.o:  
ali_main.c.o:   
ali_main.c.o:  
ali_main.c.o:  
ali_main.c.o:  
ali_main.c.o:   
ali_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ali_main.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ali_main.c.o: ali_aligner.hxx ali_arbdb.hxx ali_global.hxx ali_misc.hxx
ali_main.c.o: ali_other_stuff.hxx ali_pathmap.hxx ali_prealigner.hxx
ali_main.c.o: ali_profile.hxx ali_pt.hxx ali_sequence.hxx ali_solution.hxx
ali_main.c.o: ali_tarray.hxx ali_tlist.hxx ali_tstack.hxx
ali_pathmap.c.o:  
ali_pathmap.c.o:  
ali_pathmap.c.o: 
ali_pathmap.c.o: 
ali_pathmap.c.o:  
ali_pathmap.c.o:  
ali_pathmap.c.o:  
ali_pathmap.c.o:  
ali_pathmap.c.o:  
ali_pathmap.c.o:  
ali_pathmap.c.o:  
ali_pathmap.c.o:  
ali_pathmap.c.o:  
ali_pathmap.c.o:  
ali_pathmap.c.o:  
ali_pathmap.c.o:  
ali_pathmap.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ali_pathmap.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ali_pathmap.c.o: ali_misc.hxx ali_pathmap.hxx ali_tarray.hxx ali_tlist.hxx
ali_prealigner.c.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_prealigner.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_prealigner.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_prealigner.c.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ali_prealigner.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_prealigner.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_prealigner.c.o: $(ARBHOME)/INCLUDE/client.h
ali_prealigner.c.o: $(ARBHOME)/INCLUDE/server.h
ali_prealigner.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
ali_prealigner.c.o:  
ali_prealigner.c.o:  
ali_prealigner.c.o: 
ali_prealigner.c.o: 
ali_prealigner.c.o:  
ali_prealigner.c.o:  
ali_prealigner.c.o:  
ali_prealigner.c.o:  
ali_prealigner.c.o:  
ali_prealigner.c.o:  
ali_prealigner.c.o:  
ali_prealigner.c.o:  
ali_prealigner.c.o:  
ali_prealigner.c.o:  
ali_prealigner.c.o:  
ali_prealigner.c.o:  
ali_prealigner.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ali_prealigner.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ali_prealigner.c.o: ali_aligner.hxx ali_arbdb.hxx ali_misc.hxx
ali_prealigner.c.o: ali_other_stuff.hxx ali_pathmap.hxx ali_prealigner.hxx
ali_prealigner.c.o: ali_profile.hxx ali_pt.hxx ali_sequence.hxx
ali_prealigner.c.o: ali_solution.hxx ali_tarray.hxx ali_tlist.hxx
ali_prealigner.c.o: ali_tstack.hxx
ali_profile.c.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_profile.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_profile.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_profile.c.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ali_profile.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_profile.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_profile.c.o: $(ARBHOME)/INCLUDE/client.h
ali_profile.c.o: $(ARBHOME)/INCLUDE/server.h
ali_profile.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
ali_profile.c.o:  
ali_profile.c.o:  
ali_profile.c.o: 
ali_profile.c.o: 
ali_profile.c.o:  
ali_profile.c.o:  
ali_profile.c.o:  
ali_profile.c.o:  
ali_profile.c.o:  
ali_profile.c.o:  
ali_profile.c.o:  
ali_profile.c.o:  
ali_profile.c.o:  
ali_profile.c.o:  
ali_profile.c.o:  
ali_profile.c.o:  
ali_profile.c.o: 
ali_profile.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ali_profile.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ali_profile.c.o: BI_helix.hxx ali_arbdb.hxx ali_misc.hxx ali_other_stuff.hxx
ali_profile.c.o: ali_profile.hxx ali_pt.hxx ali_sequence.hxx ali_solution.hxx
ali_profile.c.o: ali_tlist.hxx
ali_pt.c.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_pt.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_pt.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_pt.c.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ali_pt.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_pt.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_pt.c.o: $(ARBHOME)/INCLUDE/client.h
ali_pt.c.o: $(ARBHOME)/INCLUDE/server.h
ali_pt.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
ali_pt.c.o:  
ali_pt.c.o:  
ali_pt.c.o:  
ali_pt.c.o:  
ali_pt.c.o:  
ali_pt.c.o:  
ali_pt.c.o:  
ali_pt.c.o:  
ali_pt.c.o:  
ali_pt.c.o:   
ali_pt.c.o:  
ali_pt.c.o:  
ali_pt.c.o:  
ali_pt.c.o:   
ali_pt.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ali_pt.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ali_pt.c.o: ali_misc.hxx ali_other_stuff.hxx ali_pt.hxx ali_sequence.hxx
ali_pt.c.o: ali_tlist.hxx
ali_sequence.c.o:  
ali_sequence.c.o:  
ali_sequence.c.o: 
ali_sequence.c.o: 
ali_sequence.c.o:  
ali_sequence.c.o:  
ali_sequence.c.o:  
ali_sequence.c.o:  
ali_sequence.c.o:  
ali_sequence.c.o:  
ali_sequence.c.o:  
ali_sequence.c.o:  
ali_sequence.c.o:  
ali_sequence.c.o:  
ali_sequence.c.o:  
ali_sequence.c.o:  
ali_sequence.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ali_sequence.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ali_sequence.c.o: ali_misc.hxx ali_sequence.hxx
ali_solution.c.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_solution.c.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_solution.c.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_solution.c.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ali_solution.c.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_solution.c.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_solution.c.o: $(ARBHOME)/INCLUDE/client.h
ali_solution.c.o: $(ARBHOME)/INCLUDE/server.h
ali_solution.c.o: $(ARBHOME)/INCLUDE/servercntrl.h
ali_solution.c.o:  
ali_solution.c.o:  
ali_solution.c.o: 
ali_solution.c.o: 
ali_solution.c.o:  
ali_solution.c.o:  
ali_solution.c.o:  
ali_solution.c.o:  
ali_solution.c.o:  
ali_solution.c.o:  
ali_solution.c.o:  
ali_solution.c.o:  
ali_solution.c.o:  
ali_solution.c.o:  
ali_solution.c.o:  
ali_solution.c.o:  
ali_solution.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stdarg.h
ali_solution.c.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.2/include/stddef.h
ali_solution.c.o: ali_arbdb.hxx ali_misc.hxx ali_other_stuff.hxx
ali_solution.c.o: ali_profile.hxx ali_pt.hxx ali_sequence.hxx ali_solution.hxx
ali_solution.c.o: ali_tlist.hxx
