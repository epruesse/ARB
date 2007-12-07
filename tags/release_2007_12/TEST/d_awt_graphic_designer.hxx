class AWT_graphic_designer: public AWT_graphic {
	protected:

				// internal command exec. var.
	//AW_clicked_line rot_cl;
	//AW_clicked_text rot_ct;

    public:
	// Pointer to my tree

		// *********** read only variables !!!
	GBDATA	*gb_main;

		// *********** public section
	AWT_graphic_designer(void);
	virtual ~AWT_graphic_designer(void);

	virtual	AW_gc_manager init_devices(AW_window *,AW_device *,AWT_canvas *ntw,AW_CL);

	virtual	void show(AW_device *device);
	virtual void info(AW_device *device, AW_pos x, AW_pos y,
				AW_clicked_line *cl, AW_clicked_text *ct);
	virtual void command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_key_mod modifier, AW_key_code keycode, char key_char, AW_event_type type,
				AW_pos x, AW_pos y,
				AW_clicked_line *cl, AW_clicked_text *ct);


};
