#ifndef awt_canvas_hxx_included
#define awt_canvas_hxx_included



typedef enum {
	AWT_MODE_NONE,
	AWT_MODE_SELECT,
	AWT_MODE_MARK,
	AWT_MODE_GROUP,
	AWT_MODE_ZOOM,			// no command
	AWT_MODE_LZOOM,
	AWT_MODE_MOD, // species info
	AWT_MODE_WWW,
	AWT_MODE_LINE,
	AWT_MODE_ROT,
	AWT_MODE_SPREAD,
	AWT_MODE_SWAP,
	AWT_MODE_LENGTH,
	AWT_MODE_SWAP2,
	AWT_MODE_MOVE,
	AWT_MODE_SETROOT,
	AWT_MODE_RESET,

	AWT_MODE_KERNINGHAN,
	AWT_MODE_INSERT,
	AWT_MODE_REMOVE,
	AWT_MODE_NNI,
	AWT_MODE_OPTIMIZE,
	AWT_MODE_PROINFO,
	AWT_MODE_STRETCH
} AWT_COMMAND_MODE;

class AWT_graphic_exports {
public:
    unsigned int zoom_reset:1;
    unsigned int resize:1;
    unsigned int refresh:1;
    unsigned int save:1;
    unsigned int structure_change:1; // maybe useless
    unsigned int dont_fit_x:1;
    unsigned int dont_fit_y:1;
    unsigned int dont_fit_larger:1; // if xsize>ysize -> dont_fit_x (otherwise dont_fit_y)
    unsigned int dont_scroll:1;

    void init(void); // like clear, but resets fit/scroll state
    void clear(void);

    short left_offset;
    short right_offset;
    short top_offset;
    short bottom_offset;
};

class AWT_graphic {
	friend class AWT_canvas;
protected:
    AW_rectangle extends;
    int	drag_gc;
public:
    AWT_graphic_exports	exports;

	virtual void show(AW_device *device) = 0;
	virtual void info(AW_device *device, AW_pos x, AW_pos y,		/* double click */
				AW_clicked_line *cl, AW_clicked_text *ct)= 0;
	virtual AW_gc_manager init_devices(AW_window *,AW_device *, AWT_canvas *ntw,AW_CL cd2) = 0;
			/* init gcs, if any gc is changed you may call
				AWT_expose_cb(aw_window,ntw,cd2);
				or AWT_resize_cb(aw_window,ntw,cd2);
				The function may return a pointer to a preset window */

	virtual GB_ERROR load(GBDATA *gb_main, const char *name,AW_CL cd1, AW_CL cd2);
	virtual GB_ERROR save(GBDATA *gb_main, const char *name,AW_CL cd1, AW_CL cd2);
	virtual int check_update(GBDATA *gb_main);
					// check wether anything changed
	virtual void update(GBDATA *gb_main);		// mark the database
	virtual void push_transaction(GBDATA *gb_main);
	virtual void pop_transaction(GBDATA *gb_main);


	virtual void command(AW_device *device, AWT_COMMAND_MODE cmd,
                         int button, AW_key_mod key_modifier, char key_char,
                         AW_event_type type,AW_pos x, AW_pos y,
                         AW_clicked_line *cl, AW_clicked_text *ct);
	virtual void text(AW_device *device, char *text);
	AWT_graphic(void);
	virtual ~AWT_graphic(void);
};


#define EPS 0.0001	/*div zero check*/
#define AWT_F_ALL ((AW_active)-1)
#define CLIP_OVERLAP 15
#define AWT_CATCH_LINE 50 /*pixel*/
#define AWT_CATCH_TEXT 5 /*pixel*/
#define AWT_ZOOM_OUT_STEP 40 /* (pixel) rand um screen */
#define AWT_MIN_WIDTH 100	/* Minimum center screen (= screen-offset) */
enum {
	AWT_M_LEFT = 1,
	AWT_M_MIDDLE = 2,
	AWT_M_RIGHT = 3
	};

enum {
	AWT_d_screen = 1
	};


class AWT_canvas {
private:

public:
    /** too many callbacks -> public **/
    /** in fact: private		 **/
    char *user_awar;
    void init_device(AW_device *device);
    AW_pos trans_to_fit;
    AW_pos shift_x_to_fit;
    AW_pos shift_y_to_fit;

    int old_hor_scroll_pos;
    int old_vert_scroll_pos;
    AW_rectangle rect;	// screen coordinates
    AW_world worldinfo; // real coordinates without transform.
    AW_world worldsize;
    int zoom_drag_sx;
    int zoom_drag_sy;
    int zoom_drag_ex;
    int zoom_drag_ey;
    int	drag;
    AW_clicked_line clicked_line;
    AW_clicked_text clicked_text;

    void	set_scrollbars();

    void set_horizontal_scrollbar_position(AW_window *aww, int pos);
    void set_vertical_scrollbar_position(AW_window *aww, int pos);


    /*************	Read only public section : ************/
    int               drag_gc;
    GBDATA           *gb_main;
    AW_window        *aww;
    AW_root          *awr;
    AWT_COMMAND_MODE  mode;
    AW_gc_manager     gc_manager;

    /** the real public section **/

    AWT_canvas(GBDATA *gb_main, AW_window *aww,AWT_graphic *awd, AW_gc_manager &gc_manager,const char *user_awar);
    // gc_manager is the preset window
    AWT_graphic *tree_disp;
    void	refresh();
    void	recalc_size();		// Calculate the size of the sb
    void	zoom_reset();		// Calculate all
    void	tree_zoom(AW_device *device, AW_pos sx, AW_pos sy, AW_pos ex, AW_pos ey);
    void 	scroll( AW_window *aww, int delta_x, int delta_y,
			AW_BOOL dont_update_scrollbars = AW_FALSE);
    void	set_mode(AWT_COMMAND_MODE mo);
};


void AWT_input_event(AW_window *aww, AWT_canvas *ntw, AW_CL cd2);
void AWT_motion_event(AW_window *aww, AWT_canvas *ntw, AW_CL cd2);
void AWT_expose_cb(AW_window *dummy,AWT_canvas *ntw, AW_CL cl2);
void AWT_resize_cb(AW_window *dummy,AWT_canvas *ntw, AW_CL cl2);



#define AWAR_PRINT_TREE_2_FILE_ORIENTATION	 "tmp/NT/orientation"
#define AWAR_PRINT_TREE_2_FILE_MAGNIFICATION "tmp/NT/magnification"
#define AWAR_PRINT_TREE_2_FILE_WHAT	         "tmp/NT/what"
#define AWAR_PRINT_TREE_2_FILE_HANDLES	     "tmp/NT/handles"
#define AWAR_PRINT_TREE_2_FILE_BASE	         "tmp/NT/file"
#define AWAR_PRINT_TREE_2_FILE_NAME	         AWAR_PRINT_TREE_2_FILE_BASE"/file_name"
#define AWAR_PRINT_TREE_2_FILE_DIR	         AWAR_PRINT_TREE_2_FILE_BASE"/directory"
#define AWAR_PRINT_TREE_2_FILE_FILTER        AWAR_PRINT_TREE_2_FILE_BASE"/filter"
#define AWAR_PRINT_TREE_PRINT                "tmp/NT/"

AW_window *AWT_create_export_window(AW_root *root, AWT_canvas *ntw);

AW_window *AWT_create_sec_export_window(AW_root *root, AWT_canvas *ntw);

void AWT_create_print_window(AW_window *parent_win, AWT_canvas *ntw);


#endif
