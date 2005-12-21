# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .c .depend

CPP_OBJECTS =   \
	GEN_interface.o \
	GEN_map.o \
	GEN_graphic.o \
	GEN_gene.o \
	GEN_nds.o \
	GEN_color_groups.o \
	GEN_tools.o \
    \
	EXP_interface.o \
	EXP_main.o \
	EXP_db.o \

$(MAIN): $(CPP_OBJECTS)
	$(XAR) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

.c.o:
	$(ACC) $(cflags) -c $< $(CPPINCLUDES)

clean:
	rm *.o
	rm *.a
	rm *.bak
	rm *~
	rm *#
	rm *\%

#.IGNORE:
#	clean

DEPENDS = $(CPP_OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

EXP_db.o: EXP.hxx
EXP_db.o: $(ARBHOME)/INCLUDE/ad_prot.h
EXP_db.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
EXP_db.o: $(ARBHOME)/INCLUDE/arb_assert.h
EXP_db.o: $(ARBHOME)/INCLUDE/arbdb.h
EXP_db.o: $(ARBHOME)/INCLUDE/arbdbt.h
EXP_db.o: $(ARBHOME)/INCLUDE/aw_root.hxx

EXP_interface.o: ../NTREE/ad_spec.hxx
EXP_interface.o: ../NTREE/nt_internal.h
EXP_interface.o: EXP.hxx
EXP_interface.o: EXP_interface.hxx
EXP_interface.o: EXP_local.hxx
EXP_interface.o: GEN.hxx
EXP_interface.o: GEN_interface.hxx
EXP_interface.o: GEN_local.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/ad_prot.h
EXP_interface.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
EXP_interface.o: $(ARBHOME)/INCLUDE/adGene.h
EXP_interface.o: $(ARBHOME)/INCLUDE/arb_assert.h
EXP_interface.o: $(ARBHOME)/INCLUDE/arbdb.h
EXP_interface.o: $(ARBHOME)/INCLUDE/arbdbt.h
EXP_interface.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/aw_device.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/aw_root.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/aw_window.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/awt.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/awt_changekey.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
EXP_interface.o: $(ARBHOME)/INCLUDE/db_scanner.hxx

EXP_main.o: ../NTREE/nt_cb.hxx
EXP_main.o: EXP.hxx
EXP_main.o: EXP_interface.hxx
EXP_main.o: EXP_local.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
EXP_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
EXP_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
EXP_main.o: $(ARBHOME)/INCLUDE/arbdb.h
EXP_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
EXP_main.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/awt.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/awt_hotkeys.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/awt_input_mask.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/ntree.hxx
EXP_main.o: $(ARBHOME)/INCLUDE/smartptr.h

GEN_color_groups.o: GEN_color_groups.hxx

GEN_gene.o: GEN.hxx
GEN_gene.o: GEN_gene.hxx
GEN_gene.o: GEN_local.hxx
GEN_gene.o: GEN_nds.hxx
GEN_gene.o: GEN_tools.hxx
GEN_gene.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_gene.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GEN_gene.o: $(ARBHOME)/INCLUDE/adGene.h
GEN_gene.o: $(ARBHOME)/INCLUDE/arb_assert.h
GEN_gene.o: $(ARBHOME)/INCLUDE/arbdb.h
GEN_gene.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_gene.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GEN_gene.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
GEN_gene.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GEN_gene.o: $(ARBHOME)/INCLUDE/aw_window.hxx

GEN_graphic.o: GEN.hxx
GEN_graphic.o: GEN_gene.hxx
GEN_graphic.o: GEN_graphic.hxx
GEN_graphic.o: GEN_local.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/adGene.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/arb_assert.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/arbdb.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/awt.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/awt_attributes.hxx
GEN_graphic.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx

GEN_interface.o: ../NTREE/ad_spec.hxx
GEN_interface.o: ../NTREE/nt_internal.h
GEN_interface.o: GEN.hxx
GEN_interface.o: GEN_local.hxx
GEN_interface.o: GEN_nds.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_interface.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GEN_interface.o: $(ARBHOME)/INCLUDE/adGene.h
GEN_interface.o: $(ARBHOME)/INCLUDE/arb_assert.h
GEN_interface.o: $(ARBHOME)/INCLUDE/arbdb.h
GEN_interface.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/awt.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/awt_changekey.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/db_scanner.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/ntree.hxx
GEN_interface.o: $(ARBHOME)/INCLUDE/probe_design.hxx

GEN_map.o: ../NTREE/ad_spec.hxx
GEN_map.o: EXP.hxx
GEN_map.o: EXP_interface.hxx
GEN_map.o: EXP_local.hxx
GEN_map.o: GEN.hxx
GEN_map.o: GEN_gene.hxx
GEN_map.o: GEN_graphic.hxx
GEN_map.o: GEN_interface.hxx
GEN_map.o: GEN_local.hxx
GEN_map.o: GEN_nds.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_map.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GEN_map.o: $(ARBHOME)/INCLUDE/adGene.h
GEN_map.o: $(ARBHOME)/INCLUDE/arb_assert.h
GEN_map.o: $(ARBHOME)/INCLUDE/arbdb.h
GEN_map.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_map.o: $(ARBHOME)/INCLUDE/arbtools.h
GEN_map.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_question.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/awt.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/awt_hotkeys.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/awt_input_mask.hxx
GEN_map.o: $(ARBHOME)/INCLUDE/smartptr.h

GEN_nds.o: GEN_interface.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_nds.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GEN_nds.o: $(ARBHOME)/INCLUDE/arb_assert.h
GEN_nds.o: $(ARBHOME)/INCLUDE/arbdb.h
GEN_nds.o: $(ARBHOME)/INCLUDE/arbdbt.h
GEN_nds.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/aw_window.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/awt.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/awt_changekey.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/awt_nds.hxx
GEN_nds.o: $(ARBHOME)/INCLUDE/awt_sel_boxes.hxx

GEN_tools.o: GEN.hxx
GEN_tools.o: GEN_local.hxx
GEN_tools.o: GEN_tools.hxx
GEN_tools.o: $(ARBHOME)/INCLUDE/ad_prot.h
GEN_tools.o: $(ARBHOME)/INCLUDE/adGene.h
GEN_tools.o: $(ARBHOME)/INCLUDE/arb_assert.h
GEN_tools.o: $(ARBHOME)/INCLUDE/arbdb.h
GEN_tools.o: $(ARBHOME)/INCLUDE/aw_device.hxx
GEN_tools.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
GEN_tools.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GEN_tools.o: $(ARBHOME)/INCLUDE/aw_window.hxx
