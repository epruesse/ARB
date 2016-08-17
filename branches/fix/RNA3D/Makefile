# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt
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
	$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)
.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c -o $@ $(@:.o=.cxx)  $(CXX_INCLUDES) $(POST_COMPILE) $<

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

RNA3D_Graphics.o: RNA3D_GlobalHeader.hxx
RNA3D_Graphics.o: RNA3D_Graphics.hxx
RNA3D_Graphics.o: RNA3D_Interface.hxx
RNA3D_Graphics.o: RNA3D_OpenGLEngine.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/arb_assert.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/arb_core.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/arb_error.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/arbdb_base.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/arbtools.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/attributes.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/aw_base.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/aw_device.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/aw_position.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/aw_window.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/cb.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/cb_base.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/cbtypes.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/cxxforward.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/dupstr.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/gccver.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/GL/glpng.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/smartptr.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/static_assert.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/test_global.h
RNA3D_Graphics.o: $(ARBHOME)/INCLUDE/ttypes.h

RNA3D_Interface.o: RNA3D_Global.hxx
RNA3D_Interface.o: RNA3D_GlobalHeader.hxx
RNA3D_Interface.o: RNA3D_Graphics.hxx
RNA3D_Interface.o: RNA3D_Interface.hxx
RNA3D_Interface.o: RNA3D_OpenGLEngine.hxx
RNA3D_Interface.o: RNA3D_OpenGLGraphics.hxx
RNA3D_Interface.o: RNA3D_StructureData.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/ad_prot.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/arb_assert.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/arb_core.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/arb_error.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/arb_mem.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/arb_msg.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/arb_string.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/arbdb.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/arbdb_base.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/arbtools.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/attributes.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_base.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_device.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_position.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_root.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_window.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/aw_window_Xm_interface.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/cb.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/cb_base.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/cbtypes.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/cxxforward.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/dupstr.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/gccver.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/GL/aw_window_ogl.hxx
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/GL/glpng.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/smartptr.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/static_assert.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/test_global.h
RNA3D_Interface.o: $(ARBHOME)/INCLUDE/ttypes.h

RNA3D_Main.o: rna3d_extern.hxx
RNA3D_Main.o: RNA3D_Global.hxx
RNA3D_Main.o: RNA3D_GlobalHeader.hxx
RNA3D_Main.o: RNA3D_Interface.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/ad_prot.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/arb_assert.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/arb_core.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/arb_error.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/arb_mem.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/arb_msg.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/arb_string.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/arbdb.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/arbdb_base.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/arbtools.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/attributes.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/aw_base.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/cb.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/cb_base.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/cbtypes.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/cxxforward.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/dupstr.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/ed4_plugins.hxx
RNA3D_Main.o: $(ARBHOME)/INCLUDE/gccver.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/GL/glpng.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/smartptr.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/static_assert.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/test_global.h
RNA3D_Main.o: $(ARBHOME)/INCLUDE/ttypes.h

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
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/arb_assert.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/arb_core.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/arb_error.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/arb_mem.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/arb_msg.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/arb_string.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/arbdb.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/arbdb_base.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/arbtools.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/attributes.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/aw_base.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/aw_device.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/aw_position.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/aw_root.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/aw_window.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/BI_basepos.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/cb.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/cb_base.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/cbtypes.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/cxxforward.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/dupstr.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/gccver.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/GL/aw_window_ogl.hxx
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/GL/glpng.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/smartptr.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/static_assert.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/test_global.h
RNA3D_OpenGLEngine.o: $(ARBHOME)/INCLUDE/ttypes.h

RNA3D_OpenGLGraphics.o: RNA3D_Global.hxx
RNA3D_OpenGLGraphics.o: RNA3D_GlobalHeader.hxx
RNA3D_OpenGLGraphics.o: RNA3D_OpenGLGraphics.hxx
RNA3D_OpenGLGraphics.o: RNA3D_Renderer.hxx
RNA3D_OpenGLGraphics.o: RNA3D_StructureData.hxx
RNA3D_OpenGLGraphics.o: RNA3D_Textures.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/ad_prot.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/arb_assert.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/arb_core.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/arb_error.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/arb_mem.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/arb_msg.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/arb_string.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/arbdb.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/arbdb_base.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/arbtools.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/attributes.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/aw_base.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/aw_device.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/aw_position.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/aw_window.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/aw_window_Xm_interface.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/cb.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/cb_base.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/cbtypes.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/cxxforward.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/dupstr.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/gccver.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/GL/glpng.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/smartptr.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/static_assert.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/test_global.h
RNA3D_OpenGLGraphics.o: $(ARBHOME)/INCLUDE/ttypes.h

RNA3D_Renderer.o: RNA3D_Global.hxx
RNA3D_Renderer.o: RNA3D_GlobalHeader.hxx
RNA3D_Renderer.o: RNA3D_Graphics.hxx
RNA3D_Renderer.o: RNA3D_OpenGLGraphics.hxx
RNA3D_Renderer.o: RNA3D_Renderer.hxx
RNA3D_Renderer.o: RNA3D_StructureData.hxx
RNA3D_Renderer.o: RNA3D_Textures.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/ad_prot.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/arb_assert.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/arb_core.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/arb_error.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/arb_mem.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/arb_msg.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/arb_string.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/arbdb.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/arbdb_base.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/arbtools.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/attributes.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/aw_base.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/aw_device.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/aw_position.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/aw_root.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/aw_window.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/cb.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/cb_base.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/cbtypes.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/cxxforward.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/dupstr.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/gccver.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/GL/glpng.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/smartptr.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/static_assert.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/test_global.h
RNA3D_Renderer.o: $(ARBHOME)/INCLUDE/ttypes.h

RNA3D_StructureData.o: RNA3D_Global.hxx
RNA3D_StructureData.o: RNA3D_GlobalHeader.hxx
RNA3D_StructureData.o: RNA3D_Graphics.hxx
RNA3D_StructureData.o: RNA3D_OpenGLGraphics.hxx
RNA3D_StructureData.o: RNA3D_StructureData.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/ad_prot.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/arb_assert.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/arb_core.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/arb_error.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/arb_mem.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/arb_msg.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/arb_string.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/arbdb.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/arbdb_base.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/arbdbt.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/arbtools.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/attributes.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_awar.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_awar_defs.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_base.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_device.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_device_click.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_msg.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_position.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_root.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/aw_window.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/BI_basepos.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/cb.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/cb_base.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/cbtypes.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/cxxforward.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/downcast.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/dupstr.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/ed4_extern.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/ed4_plugins.hxx
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/gccver.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/GL/glpng.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/smartptr.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/static_assert.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/test_global.h
RNA3D_StructureData.o: $(ARBHOME)/INCLUDE/ttypes.h

RNA3D_Textures.o: RNA3D_Global.hxx
RNA3D_Textures.o: RNA3D_GlobalHeader.hxx
RNA3D_Textures.o: RNA3D_OpenGLEngine.hxx
RNA3D_Textures.o: RNA3D_Textures.hxx
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/ad_prot.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/arb_assert.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/arb_core.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/arb_error.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/arb_mem.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/arb_msg.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/arb_string.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/arbdb.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/arbdb_base.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/arbtools.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/attributes.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/cxxforward.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/dupstr.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/gccver.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/GL/glpng.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/smartptr.h
RNA3D_Textures.o: $(ARBHOME)/INCLUDE/test_global.h
