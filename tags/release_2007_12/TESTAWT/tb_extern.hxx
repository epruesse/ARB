/* -----------------------------------------------------------------
 * Module:                        TESTBED/tb_extern.hxx
 *
 * Dependencies:		  uses AWT and WINDOW stuff
 *
 * Description: demonstration of AWT and WINDOW functions,
 *	        testbed for the zoomtext functions
 * -----------------------------------------------------------------
 */

/*
 * $Header$
 *
 * $Log$
 * Revision 1.3  2007/09/19 13:32:51  westram
 * - fixed command()
 *
 * Revision 1.2  2002/12/17 10:36:40  westram
 * valgrinded
 *
 * Revision 1.1.1.1  2000/11/23 09:41:16  westram
 * Erster Import
 *
 * Revision 1.1  1995/03/13  15:49:36  jakobi
 * Initial revision
 *
 * Revision 1.1  1995/03/13  15:49:36  jakobi
 * Initial revision
 *
 */


#ifndef EXTERNAL
#define EXTERNAL extern
#endif


// global variable for database
extern GBDATA *gb_main;

// visible prototypes
void tb_exit(AW_window *aw_window);
AW_window *create_tb_main_window(AW_root *awr);
AW_window *tb_preset_tree_window(AW_root *root);
void tb_reload(AW_root *, AWT_canvas *);
AWT_graphic *tb_gen_gfx(void);

// class AWT_graphic_testbed - provide the minimal necessary set of
// functions to use AWT
class AWT_graphic_testbed  : public AWT_graphic {
     public:
        // graphic styles
	AW_gc_manager init_devices(AW_window *,AW_device *,AWT_canvas *,AW_CL);
	// the testbed class
	AWT_graphic_testbed(void);
        void show(AW_device *device);
        void info(AW_device *device, AW_pos x, AW_pos y,
                                AW_clicked_line *cl, AW_clicked_text *ct);

    void command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_key_mod modifier, AW_key_code keycode, char key_char, AW_event_type type,
                                AW_pos x, AW_pos y,
                                AW_clicked_line *cl, AW_clicked_text *ct);
};

// test parameters
EXTERNAL char  *tbd_string;
EXTERNAL long  tbd_gc,tbd_function,tbd_bitset,tbd_cl1,tbd_cl2;
EXTERNAL float tbd_x,tbd_y,tbd_x0,tbd_y0,tbd_x1,tbd_y1;
EXTERNAL float tbd_height,tbd_alignment,tbd_alignmentx,tbd_alignmenty,tbd_rotation;

