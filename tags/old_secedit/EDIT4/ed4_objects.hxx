class AW_device;
class AW_event;

class ED4_object_manager;

class ED4_object {
	// Groesse


	void my_size_has_changed();				// And
	void my_graphic_has_changed();				// And

	ED4_object_manager *father;				// And
	ED4_object *next,*previous;				// And
	
		// called by parent (top down)

	virtual void calc_size	(AW_device *device);		// Seb

	virtual void show	(AW_device *device, int *color_info);	// Seb
	virtual void handle_event(AW_device *device,AW_event *event);	// Seb
	virtual void clear	(AW_device *device);		// Seb
	virtual void draw_drag_box(AW_device *device);		// Seb ???
};


class ED4_object_terminal : ED4_object {
	virtual int *get_color_info(AW_device *device);
};


class ED4_object_terminal_dbentry : ED4_object_terminal{
	GBDATA *gbd;		// Pointer to database entry
	virtual void show(AW_device *device);
	virtual int *get_color_info(AW_device *device);
};


show(AW_device *device) {
	
	device->text(0,GB_read_char_pntr(gbd), x,y, -1, 0,0);
	}
