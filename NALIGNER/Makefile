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

ali_prealigner.o:  ali_prealigner.hxx
ali_prealigner.o: ali_profile.hxx ali_sequence.hxx 
ali_prealigner.o: 
ali_prealigner.o:  
ali_prealigner.o:  
ali_prealigner.o:  
ali_prealigner.o:  
ali_prealigner.o: ali_misc.hxx 
ali_prealigner.o:  
ali_prealigner.o:  ali_arbdb.hxx ali_other_stuff.hxx
ali_prealigner.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/server.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_prealigner.o: $(ARBHOME)/INCLUDE/servercntrl.h ali_pt.hxx
ali_prealigner.o: ali_tlist.hxx $(ARBHOME)/INCLUDE/client.h
ali_prealigner.o: ali_pathmap.hxx ali_tarray.hxx ali_tstack.hxx
ali_prealigner.o: ali_solution.hxx ali_aligner.hxx
ali_aligner.o:  ali_aligner.hxx
ali_aligner.o:  
ali_aligner.o:  
ali_aligner.o:  
ali_aligner.o:  
ali_aligner.o:  ali_solution.hxx ali_profile.hxx
ali_aligner.o: ali_sequence.hxx  ali_misc.hxx
ali_aligner.o:  
ali_aligner.o:   ali_arbdb.hxx
ali_aligner.o: ali_other_stuff.hxx $(ARBHOME)/INCLUDE/PT_com.h
ali_aligner.o: $(ARBHOME)/INCLUDE/server.h
ali_aligner.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_aligner.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_aligner.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_aligner.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_aligner.o: $(ARBHOME)/INCLUDE/servercntrl.h ali_pt.hxx
ali_aligner.o: ali_tlist.hxx $(ARBHOME)/INCLUDE/client.h
ali_aligner.o: ali_tstack.hxx ali_tarray.hxx ali_pathmap.hxx
ali_arbdb.o: 
ali_arbdb.o:  
ali_arbdb.o:  
ali_arbdb.o:  
ali_arbdb.o:  
ali_arbdb.o:  
ali_arbdb.o:   ali_arbdb.hxx
ali_arbdb.o: ali_other_stuff.hxx $(ARBHOME)/INCLUDE/PT_com.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/server.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_arbdb.o: $(ARBHOME)/INCLUDE/servercntrl.h ali_sequence.hxx
ali_arbdb.o:  ali_misc.hxx 
ali_arbdb.o: 
ali_sequence.o:  
ali_sequence.o: 
ali_sequence.o:  
ali_sequence.o:  
ali_sequence.o:  
ali_sequence.o:   ali_misc.hxx
ali_sequence.o:  
ali_sequence.o:  
ali_sequence.o: ali_sequence.hxx
ali_solution.o:  
ali_solution.o:  
ali_solution.o:  
ali_solution.o:  
ali_solution.o: 
ali_solution.o:  ali_misc.hxx
ali_solution.o:  
ali_solution.o:  
ali_solution.o: ali_solution.hxx ali_profile.hxx ali_sequence.hxx
ali_solution.o:  ali_arbdb.hxx ali_other_stuff.hxx
ali_solution.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_solution.o: $(ARBHOME)/INCLUDE/server.h
ali_solution.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_solution.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_solution.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_solution.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_solution.o: $(ARBHOME)/INCLUDE/servercntrl.h ali_pt.hxx
ali_solution.o: ali_tlist.hxx $(ARBHOME)/INCLUDE/client.h
ali_pt.o: 
ali_pt.o:  
ali_pt.o:  
ali_pt.o:  
ali_pt.o:  
ali_pt.o:  
ali_pt.o:   ali_misc.hxx
ali_pt.o:  ali_pt.hxx ali_other_stuff.hxx
ali_pt.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_pt.o: $(ARBHOME)/INCLUDE/server.h
ali_pt.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_pt.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_pt.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_pt.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_pt.o: $(ARBHOME)/INCLUDE/servercntrl.h ali_tlist.hxx
ali_pt.o: ali_sequence.hxx 
ali_pt.o: $(ARBHOME)/INCLUDE/client.h
ali_global.o: 
ali_global.o:  
ali_global.o:  
ali_global.o: ali_misc.hxx  
ali_global.o:  
ali_global.o:  
ali_global.o:  
ali_global.o:  ali_global.hxx ali_arbdb.hxx
ali_global.o: ali_other_stuff.hxx $(ARBHOME)/INCLUDE/PT_com.h
ali_global.o: $(ARBHOME)/INCLUDE/server.h
ali_global.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_global.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_global.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_global.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_global.o: $(ARBHOME)/INCLUDE/servercntrl.h ali_sequence.hxx
ali_global.o:  ali_pt.hxx ali_tlist.hxx
ali_global.o: $(ARBHOME)/INCLUDE/client.h ali_profile.hxx
ali_global.o: ali_prealigner.hxx ali_pathmap.hxx ali_tarray.hxx
ali_global.o: ali_tstack.hxx ali_solution.hxx
ali_profile.o:  
ali_profile.o:  
ali_profile.o:  
ali_profile.o:  
ali_profile.o:  
ali_profile.o:  ali_misc.hxx
ali_profile.o:  
ali_profile.o:  
ali_profile.o: ali_profile.hxx ali_sequence.hxx 
ali_profile.o: ali_arbdb.hxx ali_other_stuff.hxx
ali_profile.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_profile.o: $(ARBHOME)/INCLUDE/server.h
ali_profile.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_profile.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_profile.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_profile.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_profile.o: $(ARBHOME)/INCLUDE/servercntrl.h ali_pt.hxx
ali_profile.o: ali_tlist.hxx $(ARBHOME)/INCLUDE/client.h
ali_profile.o: ali_solution.hxx BI_helix.hxx
ali_pathmap.o:  
ali_pathmap.o:  
ali_pathmap.o:  
ali_pathmap.o:  
ali_pathmap.o:  
ali_pathmap.o: ali_pathmap.hxx ali_tarray.hxx ali_misc.hxx
ali_pathmap.o:  
ali_pathmap.o:   ali_tlist.hxx
ali_main.o:  ali_misc.hxx
ali_main.o:  
ali_main.o:  
ali_main.o:  
ali_main.o:  
ali_main.o:  
ali_main.o:   ali_global.hxx
ali_main.o: ali_arbdb.hxx ali_other_stuff.hxx
ali_main.o: $(ARBHOME)/INCLUDE/PT_com.h
ali_main.o: $(ARBHOME)/INCLUDE/server.h
ali_main.o: $(ARBHOME)/INCLUDE/arbdb.h
ali_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
ali_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
ali_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ali_main.o: $(ARBHOME)/INCLUDE/servercntrl.h ali_sequence.hxx
ali_main.o:   ali_pt.hxx
ali_main.o: ali_tlist.hxx $(ARBHOME)/INCLUDE/client.h
ali_main.o: ali_profile.hxx ali_prealigner.hxx ali_pathmap.hxx ali_tarray.hxx
ali_main.o: ali_tstack.hxx ali_solution.hxx ali_aligner.hxx
BI_helix.o: 
BI_helix.o:  
BI_helix.o:  
BI_helix.o:  
BI_helix.o:  
BI_helix.o:  
BI_helix.o:  
BI_helix.o: $(ARBHOME)/INCLUDE/arbdb.h
BI_helix.o: $(ARBHOME)/INCLUDE/ad_prot.h
BI_helix.o: $(ARBHOME)/INCLUDE/arbdbt.h
BI_helix.o: $(ARBHOME)/INCLUDE/ad_t_prot.h BI_helix.hxx
