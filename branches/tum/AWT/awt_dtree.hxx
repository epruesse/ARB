#ifndef awt_dtree_hxx_included
#define awt_dtree_hxx_included

#define AWAR_DTREE_BASELINEWIDTH "awt/dtree/baselinewidth"
#define AWAR_DTREE_VERICAL_DIST	"awt/dtree/verticaldist"
#define AWAR_DTREE_AUTO_JUMP	"awt/dtree/autojump"
#define AWAR_DTREE_SHOW_CIRCLE	"awt/dtree/show_circle"
#define AWAR_DTREE_GREY_LEVEL	"awt/dtree/greylevel"

void awt_create_dtree_awars(AW_root *aw_root,AW_default def);

#define NT_BOX_WIDTH 3.5 /* pixel/2 ! */
#define NT_ROOT_WIDTH 4.5 /* pixel/2 ! */
#define NT_SELECTED_WIDTH 5.5
#define PH_CLICK_SPREAD 0.10

#define AWT_TREE(ntw) ((AWT_graphic_tree *)ntw->tree_disp)


typedef enum {
	AP_LIST_TREE,
	AP_RADIAL_TREE,
	AP_IRS_TREE,
	AP_NDS_TREE
	} AP_tree_sort;


class AWT_graphic_tree : public AWT_graphic {
	protected:

	// variables - tree compatibility

	AP_tree * tree_proto;
	double y_pos;
	double list_tree_ruler_y;
	double scale;
	AW_pos	grey_level;
				// internal command exec. var.
	double rot_orientation;
	double rot_spread;
	AW_clicked_line rot_cl;
	AW_clicked_text rot_ct;
	AW_clicked_line old_rot_cl;
	AP_tree *rot_at;

	AW_device *disp_device;	// device for  rekursiv Funktions
	void scale_text_koordinaten(AW_device *device, int gc, double& x,double& y,double orientation,int flag );

	AW_bitset line_filter,vert_line_filter, text_filter,mark_filter;
	AW_bitset ruler_filter, root_filter;	
	int treemodus;

	// functions to compute displayinformation
	double show_list_tree_rek(AP_tree * at, double x_father,
	       double x_son);
	void show_tree_rek(AP_tree *at, double x_center,
	 	double y_center,double tree_sprad,
		 double tree_orientation,
		double x_root, double y_root, int linewidth);
	void show_nds_list_rek(GBDATA * gb_main);

	void NT_scalebox(int gc, double x, double y, int width);
	void NT_emptybox(int gc, double x, double y, int width);
	void NT_rotbox(int gc, double x, double y, int width);
	const char *show_ruler(AW_device *device, int gc);
	void rot_show_triangle( AW_device *device);
	void rot_show_line( AW_device *device );
    
    void show_irs(AP_tree *at,AW_device *device, int height);
    int draw_slot(int x_offset, GB_BOOL draw_at_tips); // return max_x
    int paint_sub_tree(AP_tree *node, int x_offset, int type); // returns y pos
    
	void unload();
	char		*species_name;
	int		baselinewidth;
        int		show_circle;
    public:
		// *********** read only variables !!!
	AW_root	*aw_root;
	AP_tree_sort tree_sort;
	AP_tree * tree_root;
	AP_tree * tree_root_display;
	AP_tree_root *tree_static;
	GBDATA	*gb_main;

	AP_FLOAT x_cursor,y_cursor;
		// *********** public section
	AWT_graphic_tree(AW_root *aw_root, GBDATA *gb_main);
	virtual ~AWT_graphic_tree(void);

	void init(AP_tree * tree_prot);
	virtual	AW_gc_manager init_devices(AW_window *,AW_device *,AWT_canvas *ntw,AW_CL);

	virtual	void show(AW_device *device);
	virtual void info(AW_device *device, AW_pos x, AW_pos y,
				AW_clicked_line *cl, AW_clicked_text *ct);
	virtual void command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_event_type type, 
				AW_pos x, AW_pos y,
				AW_clicked_line *cl, AW_clicked_text *ct);

	void mark_tree(struct AP_tree *at, int mark);
	int  group_tree(struct AP_tree *at, int mode);
	int  resort_tree(int mode, struct AP_tree *at = 0 );
	void create_group(AP_tree * at);
	void toggle_group(AP_tree * at);
	void jump(AP_tree *at, const char *name);
	AP_tree *search(AP_tree *root, const char *name);
	GB_ERROR load(GBDATA *gb_main, const char *name,AW_CL link_to_database, AW_CL insert_delete_cbs);
	GB_ERROR save(GBDATA *gb_main, const char *name,AW_CL cd1, AW_CL cd2);
	int check_update(GBDATA *gb_main);	// reload tree if needed
	void update(GBDATA *gb_main);	
	void set_tree_type(AP_tree_sort type);

};

AWT_graphic *NT_generate_tree( AW_root *root,GBDATA *gb_main );




#endif
