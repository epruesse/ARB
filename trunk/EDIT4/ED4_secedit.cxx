#include <stdio.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>

#include <awt_canvas.hxx>

#include <secedit.hxx>
#include <sec_graphic.hxx>

#include "ed4_secedit.hxx"

void ED4_SECEDIT_start(AW_window *aw, AW_CL, AW_CL)
{
    static AW_window *aw_sec = 0;
    
    if (!aw_sec) { // do not open window twice
	aw_sec = SEC_create_main_window(aw->get_root());
	if (!aw_sec) {
	    aw_message("Couldn't start secondary structure editor");
	    return;
	}
    }
    aw_sec->show();
}

