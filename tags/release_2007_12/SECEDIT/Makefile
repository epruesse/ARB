# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend


CPP_OBJECTS = \
		SEC_toggle.o \
		SEC_bonddef.o \
		SEC_db.o \
		SEC_helix.o \
		SEC_graphic.o \
		SEC_structure.o \
		SEC_read.o \
		SEC_io.o \
		SEC_paint.o \
		SEC_layout.o \
		SEC_split.o \
		SEC_abspos.o \
		SEC_main.o \

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

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

SEC_abspos.o: SEC_abspos.hxx
SEC_abspos.o: SEC_defs.hxx
SEC_abspos.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEC_abspos.o: $(ARBHOME)/INCLUDE/arbtools.h

SEC_bonddef.o: SEC_bonddef.hxx
SEC_bonddef.o: SEC_defs.hxx
SEC_bonddef.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEC_bonddef.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
SEC_bonddef.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEC_bonddef.o: $(ARBHOME)/INCLUDE/arbdb.h
SEC_bonddef.o: $(ARBHOME)/INCLUDE/arbdbt.h
SEC_bonddef.o: $(ARBHOME)/INCLUDE/aw_position.hxx
SEC_bonddef.o: $(ARBHOME)/INCLUDE/aw_root.hxx

SEC_db.o: SEC_abspos.hxx
SEC_db.o: SEC_bonddef.hxx
SEC_db.o: SEC_db.hxx
SEC_db.o: SEC_defs.hxx
SEC_db.o: SEC_gc.hxx
SEC_db.o: SEC_graphic.hxx
SEC_db.o: SEC_root.hxx
SEC_db.o: SEC_toggle.hxx
SEC_db.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEC_db.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
SEC_db.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEC_db.o: $(ARBHOME)/INCLUDE/arbdb.h
SEC_db.o: $(ARBHOME)/INCLUDE/arbdbt.h
SEC_db.o: $(ARBHOME)/INCLUDE/arbtools.h
SEC_db.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
SEC_db.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
SEC_db.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEC_db.o: $(ARBHOME)/INCLUDE/aw_font_group.hxx
SEC_db.o: $(ARBHOME)/INCLUDE/aw_global.hxx
SEC_db.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
SEC_db.o: $(ARBHOME)/INCLUDE/aw_position.hxx
SEC_db.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SEC_db.o: $(ARBHOME)/INCLUDE/aw_window.hxx
SEC_db.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
SEC_db.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
SEC_db.o: $(ARBHOME)/INCLUDE/ed4_extern.hxx

SEC_graphic.o: SEC_abspos.hxx
SEC_graphic.o: SEC_db.hxx
SEC_graphic.o: SEC_defs.hxx
SEC_graphic.o: SEC_gc.hxx
SEC_graphic.o: SEC_graphic.hxx
SEC_graphic.o: SEC_iter.hxx
SEC_graphic.o: SEC_root.hxx
SEC_graphic.o: SEC_toggle.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEC_graphic.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
SEC_graphic.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEC_graphic.o: $(ARBHOME)/INCLUDE/arbdb.h
SEC_graphic.o: $(ARBHOME)/INCLUDE/arbdbt.h
SEC_graphic.o: $(ARBHOME)/INCLUDE/arbtools.h
SEC_graphic.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/aw_font_group.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/aw_global.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/aw_position.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/aw_window.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
SEC_graphic.o: $(ARBHOME)/INCLUDE/ed4_extern.hxx

SEC_helix.o: SEC_abspos.hxx
SEC_helix.o: SEC_db.hxx
SEC_helix.o: SEC_defs.hxx
SEC_helix.o: SEC_gc.hxx
SEC_helix.o: SEC_helix.hxx
SEC_helix.o: SEC_root.hxx
SEC_helix.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEC_helix.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEC_helix.o: $(ARBHOME)/INCLUDE/arbdb.h
SEC_helix.o: $(ARBHOME)/INCLUDE/arbtools.h
SEC_helix.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEC_helix.o: $(ARBHOME)/INCLUDE/aw_font_group.hxx
SEC_helix.o: $(ARBHOME)/INCLUDE/aw_position.hxx
SEC_helix.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SEC_helix.o: $(ARBHOME)/INCLUDE/BI_helix.hxx

SEC_io.o: SEC_abspos.hxx
SEC_io.o: SEC_db.hxx
SEC_io.o: SEC_defs.hxx
SEC_io.o: SEC_gc.hxx
SEC_io.o: SEC_iter.hxx
SEC_io.o: SEC_root.hxx
SEC_io.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEC_io.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEC_io.o: $(ARBHOME)/INCLUDE/arbdb.h
SEC_io.o: $(ARBHOME)/INCLUDE/arbtools.h
SEC_io.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEC_io.o: $(ARBHOME)/INCLUDE/aw_font_group.hxx
SEC_io.o: $(ARBHOME)/INCLUDE/aw_position.hxx
SEC_io.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SEC_io.o: $(ARBHOME)/INCLUDE/BI_helix.hxx

SEC_layout.o: SEC_abspos.hxx
SEC_layout.o: SEC_db.hxx
SEC_layout.o: SEC_defs.hxx
SEC_layout.o: SEC_gc.hxx
SEC_layout.o: SEC_iter.hxx
SEC_layout.o: SEC_root.hxx
SEC_layout.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEC_layout.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEC_layout.o: $(ARBHOME)/INCLUDE/arbdb.h
SEC_layout.o: $(ARBHOME)/INCLUDE/arbtools.h
SEC_layout.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEC_layout.o: $(ARBHOME)/INCLUDE/aw_font_group.hxx
SEC_layout.o: $(ARBHOME)/INCLUDE/aw_position.hxx
SEC_layout.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SEC_layout.o: $(ARBHOME)/INCLUDE/BI_helix.hxx

SEC_main.o: SEC_abspos.hxx
SEC_main.o: SEC_db.hxx
SEC_main.o: SEC_defs.hxx
SEC_main.o: SEC_drawn_pos.hxx
SEC_main.o: SEC_gc.hxx
SEC_main.o: SEC_graphic.hxx
SEC_main.o: SEC_helix.hxx
SEC_main.o: SEC_root.hxx
SEC_main.o: SEC_toggle.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEC_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
SEC_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEC_main.o: $(ARBHOME)/INCLUDE/arbdb.h
SEC_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
SEC_main.o: $(ARBHOME)/INCLUDE/arbtools.h
SEC_main.o: $(ARBHOME)/INCLUDE/aw_awars.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/aw_font_group.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/aw_position.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/aw_preset.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/aw_window.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/awt.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/ed4_extern.hxx
SEC_main.o: $(ARBHOME)/INCLUDE/FileBuffer.h

SEC_paint.o: SEC_abspos.hxx
SEC_paint.o: SEC_bonddef.hxx
SEC_paint.o: SEC_db.hxx
SEC_paint.o: SEC_defs.hxx
SEC_paint.o: SEC_drawn_pos.hxx
SEC_paint.o: SEC_gc.hxx
SEC_paint.o: SEC_graphic.hxx
SEC_paint.o: SEC_iter.hxx
SEC_paint.o: SEC_root.hxx
SEC_paint.o: SEC_toggle.hxx
SEC_paint.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEC_paint.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEC_paint.o: $(ARBHOME)/INCLUDE/arbdb.h
SEC_paint.o: $(ARBHOME)/INCLUDE/arbtools.h
SEC_paint.o: $(ARBHOME)/INCLUDE/aw_color_groups.hxx
SEC_paint.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEC_paint.o: $(ARBHOME)/INCLUDE/aw_font_group.hxx
SEC_paint.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
SEC_paint.o: $(ARBHOME)/INCLUDE/aw_position.hxx
SEC_paint.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SEC_paint.o: $(ARBHOME)/INCLUDE/aw_window.hxx
SEC_paint.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
SEC_paint.o: $(ARBHOME)/INCLUDE/awt_iupac.hxx
SEC_paint.o: $(ARBHOME)/INCLUDE/BI_helix.hxx
SEC_paint.o: $(ARBHOME)/INCLUDE/ed4_extern.hxx

SEC_read.o: SEC_abspos.hxx
SEC_read.o: SEC_db.hxx
SEC_read.o: SEC_defs.hxx
SEC_read.o: SEC_gc.hxx
SEC_read.o: SEC_iter.hxx
SEC_read.o: SEC_root.hxx
SEC_read.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEC_read.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEC_read.o: $(ARBHOME)/INCLUDE/arbdb.h
SEC_read.o: $(ARBHOME)/INCLUDE/arbtools.h
SEC_read.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEC_read.o: $(ARBHOME)/INCLUDE/aw_font_group.hxx
SEC_read.o: $(ARBHOME)/INCLUDE/aw_position.hxx
SEC_read.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SEC_read.o: $(ARBHOME)/INCLUDE/BI_helix.hxx

SEC_split.o: SEC_abspos.hxx
SEC_split.o: SEC_db.hxx
SEC_split.o: SEC_defs.hxx
SEC_split.o: SEC_drawn_pos.hxx
SEC_split.o: SEC_gc.hxx
SEC_split.o: SEC_iter.hxx
SEC_split.o: SEC_root.hxx
SEC_split.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEC_split.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEC_split.o: $(ARBHOME)/INCLUDE/arbdb.h
SEC_split.o: $(ARBHOME)/INCLUDE/arbtools.h
SEC_split.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEC_split.o: $(ARBHOME)/INCLUDE/aw_font_group.hxx
SEC_split.o: $(ARBHOME)/INCLUDE/aw_position.hxx
SEC_split.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SEC_split.o: $(ARBHOME)/INCLUDE/BI_helix.hxx

SEC_structure.o: SEC_abspos.hxx
SEC_structure.o: SEC_db.hxx
SEC_structure.o: SEC_defs.hxx
SEC_structure.o: SEC_gc.hxx
SEC_structure.o: SEC_iter.hxx
SEC_structure.o: SEC_root.hxx
SEC_structure.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEC_structure.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEC_structure.o: $(ARBHOME)/INCLUDE/arbdb.h
SEC_structure.o: $(ARBHOME)/INCLUDE/arbtools.h
SEC_structure.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEC_structure.o: $(ARBHOME)/INCLUDE/aw_font_group.hxx
SEC_structure.o: $(ARBHOME)/INCLUDE/aw_position.hxx
SEC_structure.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SEC_structure.o: $(ARBHOME)/INCLUDE/BI_helix.hxx

SEC_toggle.o: SEC_defs.hxx
SEC_toggle.o: SEC_graphic.hxx
SEC_toggle.o: SEC_toggle.hxx
SEC_toggle.o: $(ARBHOME)/INCLUDE/ad_prot.h
SEC_toggle.o: $(ARBHOME)/INCLUDE/arb_assert.h
SEC_toggle.o: $(ARBHOME)/INCLUDE/arbdb.h
SEC_toggle.o: $(ARBHOME)/INCLUDE/aw_device.hxx
SEC_toggle.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
SEC_toggle.o: $(ARBHOME)/INCLUDE/aw_position.hxx
SEC_toggle.o: $(ARBHOME)/INCLUDE/aw_root.hxx
SEC_toggle.o: $(ARBHOME)/INCLUDE/aw_window.hxx
SEC_toggle.o: $(ARBHOME)/INCLUDE/awt_canvas.hxx
