#ifndef aw_device_hxx_included
#define aw_device_hxx_included

#define _AW_DEVICE_INCLUDED

#ifndef _AW_COMMON_INCLUDED
class AW_common { void *dummy;};
#endif

#if defined(DEBUG) && defined(DEBUG_GRAPHICS)
// if you want flush() to be called after every motif command :
#define AUTO_FLUSH(device) (device)->flush()
//#define AUTO_FLUSH(device)
#else
#define AUTO_FLUSH(device)
#endif

#define AW_PIXELS_PER_MM 1.0001

const AW_bitset AW_SCREEN      = 1;
const AW_bitset AW_CLICK       = 2;
const AW_bitset AW_CLICK_DRAG  = 4;
const AW_bitset AW_SIZE        = 8;
const AW_bitset AW_PRINTER     = 16;
const AW_bitset AW_PRINTER_EXT = 32; // Handles ...

typedef enum {
    AW_DEVICE_SCREEN  = 1,
    AW_DEVICE_CLICK   = 2,
    AW_DEVICE_SIZE    = 8,
    AW_DEVICE_PRINTER = 16
} AW_DEVICE_TYPE;

typedef enum {
    AW_INFO_AREA,
    AW_MIDDLE_AREA,
    AW_BOTTOM_AREA,
    AW_MAX_AREA
} AW_area;

enum {
    AW_FIXED             = -1,
    AW_TIMES             = 0,
    AW_TIMES_ITALIC      = 1,
    AW_TIMES_BOLD        = 2,
    AW_TIMES_BOLD_ITALIC = 3,

    AW_COURIER              = 12,
    AW_COURIER_OBLIQUE      = 13,
    AW_COURIER_BOLD         = 14,
    AW_COURIER_BOLD_OBLIQUE = 15,

    AW_HELVETICA                     = 16,
    AW_HELVETICA_OBLIQUE             = 17,
    AW_HELVETICA_BOLD                = 18,
    AW_HELVETICA_BOLD_OBLIQUE        = 19,
    AW_HELVETICA_NARROW              = 20,
    AW_HELVETICA_NARROW_OBLIQUE      = 21,
    AW_HELVETICA_NARROW_BOLD         = 22,
    AW_HELVETICA_NARROW_BOLD_OBLIQUE = 23,

    AW_LUCIDA_SANS                 = 35,
    AW_LUCIDA_SANS_OBLIQUE         = 36,
    AW_LUCIDA_SANS_BOLD            = 37,
    AW_LUCIDA_SANS_BOLD_OBLIQUE    = 38,
    AW_LUCIDA_SANS_TYPEWRITER      = 39,
    AW_LUCIDA_SANS_TYPEWRITER_BOLD = 40,
    AW_NUM_FONTS                   = 41,

    AW_DEFAULT_NORMAL_FONT = AW_LUCIDA_SANS,
    AW_DEFAULT_BOLD_FONT   = AW_LUCIDA_SANS_BOLD,
    AW_DEFAULT_FIXED_FONT  = AW_LUCIDA_SANS_TYPEWRITER,

};      // AW_font

typedef enum {
    AW_WINDOW_BG,
    AW_WINDOW_FG,
    AW_WINDOW_C1,
    AW_WINDOW_C2,
    AW_WINDOW_C3,
    AW_WINDOW_DRAG,
    AW_DATA_BG,
    AW_COLOR_MAX
} AW_color;

typedef enum {
    AW_cursor_insert,
    AW_cursor_overwrite
} AW_cursor_type;

class AW_clicked_line {
public:
    AW_pos  x0,y0,x1,y1;
    AW_pos  height;
    AW_pos  length;
    AW_CL   client_data1;
    AW_CL   client_data2;
    AW_BOOL exists;
};

class AW_clicked_text {
public:
    AW_pos  x0, y0;         // start of text
    AW_pos  alignment;
    AW_pos  rotation;
    AW_pos  distance;       // Entfernung zum Text, <0 => oberhalb, >0 => unterhalb
    int     cursor;         // which letter was selected, from 0 to strlen-1
    AW_CL   client_data1;
    AW_CL   client_data2;
    AW_BOOL exists;         // AW_TRUE if a text was clicked, else AW_FALSE
};

class AW_matrix {
    friend class AW_device;
    AW_pos xoffset,yoffset;
    AW_pos scale;
public:
    AW_matrix(void) { this->reset();};
    virtual ~AW_matrix() {}

    void zoom(AW_pos scale);
    AW_pos get_scale() { return scale; };
    void rotate(AW_pos angle);
    void shift_x(AW_pos xoff);
    void shift_y(AW_pos yoff);
    void shift_dx(AW_pos xoff);
    void shift_dy(AW_pos yoff);
    void reset(void);

    void transform(int x,int y,int& xout,int& yout) {
        xout = int((x+ xoffset)*scale);
        yout = int((y + yoffset)*scale);
    }
    void transform(AW_pos x,AW_pos y,AW_pos& xout,AW_pos& yout) {
        xout = (x+ xoffset)*scale;
        yout = (y + yoffset)*scale;
    }
    void rtransform(int x,int y,int& xout,int& yout) {
        xout = int(x/scale - xoffset);
        yout = int(y/scale - yoffset);
    }
    void rtransform(AW_pos x,AW_pos y,AW_pos& xout,AW_pos& yout) {
        xout = x/scale - xoffset;
        yout = y/scale - yoffset;
    }
};

class AW_common;

class AW_clip {
    friend class AW_device;
protected:
    int compoutcode(AW_pos xx, AW_pos yy) {
        /* calculate outcode for clipping the current line */
        /* order - top,bottom,right,left */
        register int code = 0;
        if (clip_rect.b - yy < 0)       code = 4;
        else if (yy - clip_rect.t < 0)  code = 8;
        if (clip_rect.r - xx < 0)       code |= 2;
        else if (xx - clip_rect.l < 0)  code |= 1;
        return(code);
    };
public:
    class AW_common *common;

    // ****** read only section
    AW_rectangle clip_rect;     //holds the clipping rectangle coordinates
    int top_font_overlap;
    int bottom_font_overlap;
    int left_font_overlap;
    int right_font_overlap;

    // ****** real public
    int clip(AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_pos& x0out, AW_pos& y0out, AW_pos& x1out, AW_pos& y1out);
    int box_clip(AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_pos& x0out, AW_pos& y0out, AW_pos& x1out, AW_pos& y1out);


    void set_top_clip_border(int top, AW_BOOL allow_oversize = AW_FALSE);
    void set_bottom_clip_border(int bottom, AW_BOOL allow_oversize = AW_FALSE); // absolut
    void set_bottom_clip_margin(int bottom, AW_BOOL allow_oversize = AW_FALSE); // relativ
    void set_left_clip_border(int left, AW_BOOL allow_oversize = AW_FALSE);
    void set_right_clip_border(int right, AW_BOOL allow_oversize = AW_FALSE);
    void set_cliprect(AW_rectangle *rect, AW_BOOL allow_oversize = AW_FALSE);

    void set_top_font_overlap(int val=1);
    void set_bottom_font_overlap(int val=1);
    void set_left_font_overlap(int val=1);
    void set_right_font_overlap(int val=1);

    // like set_xxx_clip_border but make window only smaller:

    void reduce_top_clip_border(int top);
    void reduce_bottom_clip_border(int bottom);
    void reduce_left_clip_border(int left);
    void reduce_right_clip_border(int right);

    void reduceClipBorders(int top, int bottom, int left, int right);

    AW_clip();
    virtual ~AW_clip() {}
};

class AW_clip_scale_stack {
    friend class AW_device;

    AW_rectangle clip_rect;

    int top_font_overlap;
    int bottom_font_overlap;
    int left_font_overlap;
    int right_font_overlap;

    AW_pos xoffset,yoffset;
    AW_pos scale;

    class AW_clip_scale_stack *next;
};




class AW_font_information {
public:
    short max_letter_ascent;
    short max_letter_descent;
    short max_letter_height;
    short max_letter_width;
    short this_letter_ascent;
    short this_letter_descent;
    short this_letter_height;
    short this_letter_width;
};


/***************************************************************************************************
 ***********                     Graphic Context (Linestyle, width ...                   ************
 ***************************************************************************************************/
typedef enum {
    AW_SOLID,
    AW_DOTTED
} AW_linestyle;


typedef enum {
    AW_COPY,
    AW_XOR
} AW_function;

class AW_gc: public AW_clip {
public:
    void    new_gc(int gc);
    int     new_gc(void);
    void    set_fill(int gc,AW_grey_level grey_level);      // <0 dont fill  0.0 white 1.0 black
    void    set_font(int gc,AW_font fontnr, int size);      //
    void    set_line_attributes(int gc,AW_pos width,AW_linestyle style);
    void    set_function(int gc,AW_function function);
    void    set_foreground_color(int gc,AW_color color);    // lines ....
    void    set_background_color(int gc,AW_color color);    // for box
    AW_font_information     *get_font_information(int gc, unsigned char c);
    int     get_string_size(int gc,const  char *string,long textlen);       // get the size of the string
    AW_gc();
    virtual ~AW_gc() {};
};

/***************************************************************************************************
 ***********                     The abstract class AW_device ...                        ************
 ***************************************************************************************************/

class AW_device: public AW_matrix, public AW_gc {
     AW_device(const AW_device& other);
     AW_device& operator=(const AW_device& other);
 protected:
     AW_clip_scale_stack *clip_scale_stack;
     virtual         void  _privat_reset(void);

 public:
     AW_device(AW_common *common); // get the device from  AW_window
     virtual ~AW_device() {}
     // by device = get_device(area);
     /***************** The Read Only  Section ******************/
     AW_bitset filter;
     /***************** The real Public Section ******************/

     void reset(void);
     void get_area_size(AW_rectangle *rect); //read the frame size
     void get_area_size(AW_world *rect); //read the frame size
     void set_filter( AW_bitset filteri ); //set the main filter mask

     void push_clip_scale(void); // push clipping area and scale
     void pop_clip_scale(void); // pop them

     virtual AW_DEVICE_TYPE type(void) =0;

     // * functions below return 1 if any pixel is drawn, 0 otherwise     
     // * primary functions (always virtual)
     
     virtual bool ready_to_draw(int gc);

     virtual int line(int gc, AW_pos x0,AW_pos y0, AW_pos x1,AW_pos y1,
                      AW_bitset filteri = (AW_bitset)-1, AW_CL cd1 = 0, AW_CL cd2 = 0) = 0; // used by click device

     virtual int text(int gc, const char *string,AW_pos x,AW_pos y,
                      AW_pos alignment  = 0.0, // 0.0 alignment left 0.5 centered 1.0 right justified
                      AW_bitset filteri    = (AW_bitset)-1, AW_CL cd1 = 0, AW_CL cd2 = 0, // used by click device
                      long opt_strlen = 0) = 0;

     // * second level functions (maybe non virtual)

     virtual int invisible(int gc, AW_pos x, AW_pos y, AW_bitset filteri, AW_CL cd1, AW_CL cd2); // returns 1 when invisible would be on screen
     virtual int cursor(int gc, AW_pos x0,AW_pos y0, AW_cursor_type type, AW_bitset filteri, AW_CL cd1, AW_CL cd2);

     virtual int zoomtext(int gc, const char *string, AW_pos x,AW_pos y, AW_pos height, AW_pos alignment,AW_pos rotation,AW_bitset filteri,AW_CL cd1,AW_CL cd2);
     virtual int zoomtext1(int gc, const char *string, AW_pos x,AW_pos y, AW_pos scale, AW_pos alignment,AW_pos rotation, AW_bitset filteri,AW_CL cd1,AW_CL cd2);
     virtual int zoomtext4line(int gc, const char *string, AW_pos height, AW_pos lx0, AW_pos ly0, AW_pos lx1, AW_pos ly1, AW_pos alignmentx, AW_pos alignmenty, AW_bitset filteri,AW_CL cd1,AW_CL cd2);


     virtual int box(int gc, AW_pos x0,AW_pos y0,AW_pos width,AW_pos heigth, AW_bitset filteri, AW_CL cd1, AW_CL cd2);
     virtual int circle(int gc, AW_BOOL filled, AW_pos x0,AW_pos y0,AW_pos width,AW_pos heigth, AW_bitset filter, AW_CL cd1, AW_CL cd2);
     virtual int filled_area(int gc, int npoints, AW_pos *points, AW_bitset filteri, AW_CL cd1, AW_CL cd2);

     // * third level functions (never virtual)

     // reduces any string (or virtual string) to its actual drawn size and calls the function f with the result
     int     text_overlay( int gc, const char *opt_string, long opt_strlen,  // either string or strlen != 0
                           AW_pos x,AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL cduser, AW_CL cd1, AW_CL cd2,
                           AW_pos opt_ascent,AW_pos opt_descent,   // optional height (if == 0 take font height)
                           int (*f)(AW_device *device, int gc, const char *opt_string, size_t opt_string_len, size_t start, size_t size,
                                    AW_pos x,AW_pos y, AW_pos opt_ascent,AW_pos opt_descent,
                                    AW_CL cduser, AW_CL cd1, AW_CL cd2));


     // ********* X11 Device only ********
     virtual void    clear(void);
     virtual void    clear_part(AW_pos x, AW_pos y, AW_pos width, AW_pos height);
     virtual void    clear_text(int gc, const char *string, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL cd1, AW_CL cd2);
     virtual void    move_region( AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height, AW_pos dest_x, AW_pos dest_y );
     virtual void    fast(void);                                     // e.g. zoom linewidth off
     virtual void    slow(void);
     virtual void    flush(void);                                    // empty X11 buffers
     // ********* click device only ********
     virtual void    get_clicked_line(AW_clicked_line *ptr);
     virtual void    get_clicked_text(AW_clicked_text *ptr);
     // ********* size device only ********
     virtual void    get_size_information(AW_world *ptr);
     // ********* print device only (xfig compatible) ********
     virtual const char *open(const char *path);
     virtual void    close(void);
 };



#endif
