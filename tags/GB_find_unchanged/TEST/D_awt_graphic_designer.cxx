#include <stdio.h>


#include <arbdb.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_preset.hxx>
#include <awt.hxx>
#include <awt_canvas.hxx>
#include <awt_preset.hxx>
#include <d_awt_graphic_designer.hxx>

enum {
	AWT_GC_CURSOR=0,
	AWT_GC_SELECTED,		// == zero mismatches
	AWT_GC_UNDIFF,
	AWT_GC_NSELECTED,		// no hit
	AWT_GC_SOME_MISMATCHES,
	AWT_GC_MAX
}; // AW_gc

AW_gc_manager AWT_graphic_designer::init_devices(AW_window *aww, AW_device *device, AWT_canvas* ntw, AW_CL cd2) {

	AW_gc_manager preset_window =
		AW_manage_GC(aww,device,AWT_GC_CURSOR, AWT_GC_MAX, AW_GCM_DATA_AREA,
                     (AW_CB)AWT_resize_cb, (AW_CL)ntw, cd2,
                     false, // no color groups
                     "",
                     "CURSOR",
                     "SELECTED",
                     "SEL. & NOT",
                     "NOT SEL.",
                     "ETC",
                     NULL);

	return preset_window;
}



void AWT_graphic_designer::show(AW_device *device)	{

	//AW_font_information *fontinfo = disp_device->get_font_information(AWT_GC_SELECTED, 0);
	//scale = fontinfo->max_letter_height/ device->get_scale();


	device->line(AWT_GC_CURSOR, 1, 1, 200, 200, -1, 0, 0);


}

AWT_graphic_designer::AWT_graphic_designer() {
		exports.dont_fit_x = 0;
		exports.dont_fit_y = 0;
		exports.left_offset = 150;
		exports.right_offset = 150;
		exports.top_offset = 30;
		exports.bottom_offset = 30;
;}

AWT_graphic_designer::~AWT_graphic_designer() {;}

void AWT_graphic_designer::command(AW_device*, AWT_COMMAND_MODE, int, AW_key_mod, char, AW_event_type, double, double,
				AW_clicked_line*, AW_clicked_text*) {;}


void AWT_graphic_designer::info(AW_device*, double, double, AW_clicked_line*, AW_clicked_text*) {;}
