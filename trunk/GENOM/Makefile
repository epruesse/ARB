# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx


CPP_OBJECTS =   \
	GEN_read.o \
	GEN_db.o \
	GEN_interface.o \
	GEN_map.o \
	GEN_graphic.o \
	GEN_gene.o \
	GEN_nds.o \
	GEN_color_groups.o \

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

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

GEN_read.o: $(ARBHOME)/INCLUDE/arbdb.h 
GEN_read.o:  
GEN_read.o:  
GEN_read.o:  
GEN_read.o:  $(ARBHOME)/INCLUDE/ad_prot.h
GEN_read.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_read.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GEN_read.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GEN_read.o: $(ARBHOME)/INCLUDE/awtc_rename.hxx GEN_local.hxx GEN.hxx
GEN_read.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GEN_read.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GEN_db.o:  $(ARBHOME)/INCLUDE/arbdb.h
GEN_db.o:  
GEN_db.o:  
GEN_db.o:  
GEN_db.o:  
GEN_db.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_db.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_db.o: $(ARBHOME)/INCLUDE/ad_t_prot.h GEN_local.hxx GEN.hxx
GEN_db.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GEN_db.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/awt.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/arbdb.h 
GEN_interface.o:  
GEN_interface.o:  
GEN_interface.o:  
GEN_interface.o: 
GEN_interface.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_interface.o: $(ARBHOME)/INCLUDE/ntree.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_interface.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GEN_interface.o: $(ARBHOME)/INCLUDE/probe_design.hxx
GEN_interface.o: ./../NTREE/ad_spec.hxx GEN_local.hxx GEN.hxx GEN_nds.hxx
GEN_map.o:  
GEN_map.o:  
GEN_map.o:  
GEN_map.o:  
GEN_map.o:   
GEN_map.o:  
GEN_map.o:  
GEN_map.o:  
GEN_map.o:  
GEN_map.o: $(ARBHOME)/INCLUDE/arbdb.h
GEN_map.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_map.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_map.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GEN_map.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/awt.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/awtc_rename.hxx GEN_local.hxx GEN.hxx
GEN_map.o: GEN_gene.hxx GEN_graphic.hxx GEN_nds.hxx ./../NTREE/ad_spec.hxx
GEN_graphic.o:  
GEN_graphic.o:  
GEN_graphic.o:  
GEN_graphic.o:  
GEN_graphic.o:  
GEN_graphic.o:  
GEN_graphic.o:  
GEN_graphic.o:  
GEN_graphic.o:  
GEN_graphic.o:  
GEN_graphic.o:  
GEN_graphic.o: 
GEN_graphic.o: $(ARBHOME)/INCLUDE/arbdb.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/awt.hxx GEN_local.hxx GEN.hxx
GEN_graphic.o: GEN_gene.hxx GEN_graphic.hxx
GEN_gene.o:  
GEN_gene.o:  
GEN_gene.o:  
GEN_gene.o:  
GEN_gene.o:  
GEN_gene.o:  
GEN_gene.o:  
GEN_gene.o:  
GEN_gene.o:  
GEN_gene.o:  $(ARBHOME)/INCLUDE/arbdb.h
GEN_gene.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_gene.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_gene.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GEN_gene.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GEN_gene.o: $(ARBHOME)/INCLUDE/aw_device.hxx GEN.hxx
GEN_gene.o: $(ARBHOME)/INCLUDE/aw_window.hxx GEN_gene.hxx
GEN_gene.o: GEN_local.hxx GEN_nds.hxx
GEN_nds.o:  
GEN_nds.o:  
GEN_nds.o:  
GEN_nds.o:  
GEN_nds.o:   
GEN_nds.o:  
GEN_nds.o:  
GEN_nds.o:  
GEN_nds.o:  
GEN_nds.o: $(ARBHOME)/INCLUDE/arbdb.h
GEN_nds.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_nds.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_nds.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GEN_nds.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/awt.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/awt_nds.hxx GEN_interface.hxx
GEN_color_groups.o: GEN_color_groups.hxx
