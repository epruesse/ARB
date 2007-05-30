# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx .depend

CPP_OBJECTS = 	RNA3D_Main.o \
				RNA3D_Interface.o \
				RNA3D_Graphics.o \
				RNA3D_OpenGLEngine.o \
				RNA3D_OpenGLGraphics.o \
				RNA3D_Textures.o \
				RNA3D_StructureData.o \
				RNA3D_Renderer.o 

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)
.cxx.o:
	$(CPP) $(cflags) -c -o $@ $(@:.o=.cxx)  $(CPPINCLUDES)

clean:
	rm -f $(CPP_OBJECTS) *.a

DEPENDS = $(CPP_OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

RNA3D_Graphics.o: RNA3D_GlobalHeader.hxx
RNA3D_Graphics.o: RNA3D_Graphics.hxx
RNA3D_Graphics.o: RNA3D_Interface.hxx
RNA3D_Graphics.o: RNA3D_OpenGLEngine.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/ad_prot.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/arb_assert.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/arbdb.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/arbdbt.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/aw_device.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/aw_root.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/aw_window.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/awt.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/awt_tree_cb.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/GL/glpng.h

RNA3D_Interface.o: ../WINDOW/aw_awar.hxx
RNA3D_Interface.o: ../WINDOW/aw_click.hxx
RNA3D_Interface.o: ../WINDOW/aw_print.hxx
RNA3D_Interface.o: ../WINDOW/aw_size.hxx
RNA3D_Interface.o: ../WINDOW/aw_window_Xm.hxx
RNA3D_Interface.o: ../WINDOW/aw_Xm.hxx
RNA3D_Interface.o: RNA3D_Global.hxx
RNA3D_Interface.o: RNA3D_GlobalHeader.hxx
RNA3D_Interface.o: RNA3D_Graphics.hxx
RNA3D_Interface.o: RNA3D_Interface.hxx
RNA3D_Interface.o: RNA3D_OpenGLEngine.hxx
RNA3D_Interface.o: RNA3D_OpenGLGraphics.hxx
RNA3D_Interface.o: RNA3D_StructureData.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/ad_prot.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/arb_assert.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/arbdb.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/arbdbt.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_device.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_root.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_window.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/awt.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/awt_tree_cb.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/GL/glpng.h

RNA3D_Main.o: RNA3D_Global.hxx
RNA3D_Main.o: RNA3D_GlobalHeader.hxx
RNA3D_Main.o: RNA3D_Interface.hxx
RNA3D_Main.o: RNA3D_Main.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/ad_prot.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/arb_assert.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/arbdb.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/arbdbt.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/awt.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/awt_tree_cb.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/GL/glpng.h

RNA3D_OpenGLEngine.o: RNA3D_Global.hxx
RNA3D_OpenGLEngine.o: RNA3D_GlobalHeader.hxx
RNA3D_OpenGLEngine.o: RNA3D_Graphics.hxx
RNA3D_OpenGLEngine.o: RNA3D_Interface.hxx
RNA3D_OpenGLEngine.o: RNA3D_OpenGLEngine.hxx
RNA3D_OpenGLEngine.o: RNA3D_OpenGLGraphics.hxx
RNA3D_OpenGLEngine.o: RNA3D_Renderer.hxx
RNA3D_OpenGLEngine.o: RNA3D_StructureData.hxx
RNA3D_OpenGLEngine.o: RNA3D_Textures.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/ad_prot.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/arb_assert.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/arbdb.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/arbdbt.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/aw_device.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/aw_root.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/aw_window.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/awt.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/awt_tree_cb.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/GL/glpng.h

RNA3D_OpenGLGraphics.o: ../WINDOW/aw_awar.hxx
RNA3D_OpenGLGraphics.o: ../WINDOW/aw_click.hxx
RNA3D_OpenGLGraphics.o: ../WINDOW/aw_commn.hxx
RNA3D_OpenGLGraphics.o: ../WINDOW/aw_print.hxx
RNA3D_OpenGLGraphics.o: ../WINDOW/aw_size.hxx
RNA3D_OpenGLGraphics.o: ../WINDOW/aw_window_Xm.hxx
RNA3D_OpenGLGraphics.o: ../WINDOW/aw_Xm.hxx
RNA3D_OpenGLGraphics.o: RNA3D_Global.hxx
RNA3D_OpenGLGraphics.o: RNA3D_GlobalHeader.hxx
RNA3D_OpenGLGraphics.o: RNA3D_OpenGLGraphics.hxx
RNA3D_OpenGLGraphics.o: RNA3D_Renderer.hxx
RNA3D_OpenGLGraphics.o: RNA3D_StructureData.hxx
RNA3D_OpenGLGraphics.o: RNA3D_Textures.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/ad_prot.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/arb_assert.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/arbdb.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/arbdbt.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/aw_device.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/aw_root.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/aw_window.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/awt.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/awt_tree_cb.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/GL/glpng.h

RNA3D_Renderer.o: RNA3D_Global.hxx
RNA3D_Renderer.o: RNA3D_GlobalHeader.hxx
RNA3D_Renderer.o: RNA3D_Graphics.hxx
RNA3D_Renderer.o: RNA3D_OpenGLGraphics.hxx
RNA3D_Renderer.o: RNA3D_Renderer.hxx
RNA3D_Renderer.o: RNA3D_StructureData.hxx
RNA3D_Renderer.o: RNA3D_Textures.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/ad_prot.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/arb_assert.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/arbdb.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/arbdbt.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/aw_device.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/aw_root.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/aw_window.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/awt.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/awt_tree_cb.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/GL/glpng.h

RNA3D_StructureData.o: ../EDIT4/ed4_class.hxx
RNA3D_StructureData.o: ../EDIT4/ed4_defs.hxx
RNA3D_StructureData.o: ../EDIT4/ed4_defs.hxx
RNA3D_StructureData.o: ../EDIT4/ed4_search.hxx
RNA3D_StructureData.o: ../EDIT4/ed4_visualizeSAI.hxx
RNA3D_StructureData.o: RNA3D_Global.hxx
RNA3D_StructureData.o: RNA3D_GlobalHeader.hxx
RNA3D_StructureData.o: RNA3D_Graphics.hxx
RNA3D_StructureData.o: RNA3D_OpenGLGraphics.hxx
RNA3D_StructureData.o: RNA3D_StructureData.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/ad_prot.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/arb_assert.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/arbdb.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/arbdbt.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_device.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_font_group.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_root.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_window.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/awt.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/awt_tree_cb.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/GL/glpng.h

RNA3D_Textures.o: RNA3D_Global.hxx
RNA3D_Textures.o: RNA3D_GlobalHeader.hxx
RNA3D_Textures.o: RNA3D_OpenGLEngine.hxx
RNA3D_Textures.o: RNA3D_Textures.hxx
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/ad_prot.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/arb_assert.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/arbdb.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/arbdbt.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/aw_device.hxx
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/aw_root.hxx
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/aw_window.hxx
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/awt.hxx
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/awt_dtree.hxx
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/awt_pro_a_nucs.hxx
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/awt_tree.hxx
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/awt_tree_cb.hxx
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/GL/glpng.h
