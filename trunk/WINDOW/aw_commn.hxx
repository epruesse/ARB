#ifndef AW_COMMN_HXX
#define AW_COMMN_HXX

// #define _AW_COMMON_INCLUDED

#define AW_INT(x) (  ((x)>=0) ? (int) ((x)+.5):(int )((x)-.5) )

class AW_GC_Xm {
public:
	GC			gc;
	class AW_common		*common;
	XFontStruct		curfont;
	short			width_of_chars[256];
	short			ascent_of_chars[256];
	short			descent_of_chars[256];
	AW_font_information	fontinfo;
	short			line_width;
	AW_linestyle 		style;
	short			color;

	short			fontsize;
	AW_font			fontnr;

	AW_function function;
	AW_pos			grey_level;
	AW_GC_Xm(class AW_common	*common);
	~AW_GC_Xm();
	void set_fill(AW_grey_level grey_level);								// <0 dont fill	 0.0 white 1.0 black
	void set_font(AW_font font_nr, int size);
	void set_lineattributes(AW_pos width, AW_linestyle style);
	void set_function(AW_function function);
	void set_foreground_color(unsigned long color);
	void set_background_color(unsigned long color);
};


class AW_common {
	public:
	AW_common(AW_window *aww, AW_area area,	Display *display_in,XID window_id_in,unsigned long *fcolors,unsigned int **dcolors);
	unsigned long *frame_colors;
	unsigned long **data_colors;
	AW_root		*root;
	AW_rectangle	screen;
	int		screen_x_offset;
	int		screen_y_offset;
	AW_GC_Xm	**gcs;
	int		ngcs;
	Display		*display;
	XID		window_id;

	AW_pos	x_alignment(AW_pos x_pos,AW_pos x_size,AW_pos alignment)
		{ return x_pos- x_size*alignment; };
};


// #define AW_MAP_GC(gc) (aw_assert(gc<common->ngcs), common->gcs[gc])

inline bool AW_GC_MAPABLE(AW_common *common, int gc) {
    return gc<common->ngcs && common->gcs[gc] != 0;
}

inline AW_GC_Xm *AW_MAP_GC_tested(AW_common *common, int gc) {
    aw_assert(AW_GC_MAPABLE(common, gc));
    return common->gcs[gc];
}
#define AW_MAP_GC(gc) AW_MAP_GC_tested(common, gc)

#else
#error aw_commn.hxx included twice
#endif
