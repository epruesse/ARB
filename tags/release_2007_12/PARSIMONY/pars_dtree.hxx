class AWT_graphic_parsimony: public AWT_graphic_tree {
	virtual AW_gc_manager init_devices(AW_window *,AW_device *, AWT_canvas *ntw,AW_CL cd2);
			/* init gcs, if any gc is changed you may call
				AWT_expose_cb(aw_window,ntw,cd2);
				or AWT_resize_cb(aw_window,ntw,cd2);
				The function may return a pointer to a preset window */


	virtual	void show(AW_device *device);
	virtual void command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_key_mod key_modifier, AW_key_code key_code, char key_char,
				AW_event_type type,AW_pos x, AW_pos y,
				AW_clicked_line *cl, AW_clicked_text *ct);
	public:
		AWT_graphic_parsimony(AW_root *root, GBDATA *gb_main);

};

void NT_tree_init(AWT_graphic_tree *agt);
void PARS_optimizer_cb(AP_tree *tree);
