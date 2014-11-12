module:=aw
aw_TARGETS=lib/libWINDOW.so
aw_CPPFLAGS=-DIN_ARB_WINDOW
aw_LIBADD:=db
aw_LDFLAGS:=$(XLIBS)

aw_HEADERS := aw_advice.hxx aw_awar.hxx aw_awar_defs.hxx aw_awars.hxx
aw_HEADERS += aw_base.hxx aw_color_groups.hxx aw_device.hxx
aw_HEADERS += aw_device_click.hxx aw_edit.hxx aw_file.hxx
aw_HEADERS += aw_font_group.hxx aw_global.hxx aw_global_awars.hxx
aw_HEADERS += aw_keysym.hxx aw_msg.hxx aw_position.hxx aw_preset.hxx
aw_HEADERS += aw_question.hxx aw_root.hxx aw_scalar.hxx aw_select.hxx
aw_HEADERS += aw_window.hxx aw_window_Xm_interface.hxx

