#ifndef AW_CLICK_HXX
#define AW_CLICK_HXX

class AW_device_click: public AW_device {
protected:
    AW_pos          mouse_x,mouse_y;
    AW_pos          max_distance_line;
    AW_pos          max_distance_text;

public:
    AW_clicked_line opt_line;
    AW_clicked_text opt_text;
    
    AW_device_click(AW_common *commoni);
    
    AW_DEVICE_TYPE type(void);
    
    void init(AW_pos mousex, AW_pos mousey, AW_pos max_distance_liniei, AW_pos max_distance_texti, AW_pos radi, AW_bitset filteri);

    int line(int gc, AW_pos x0,AW_pos y0, AW_pos x1,AW_pos y1, AW_bitset filteri, AW_CL clientdata1, AW_CL clientdata2);
    int text(int gc, const char *string,AW_pos x,AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL clientdata1, AW_CL clientdata2, long opt_strlen);
    
    void get_clicked_line(class AW_clicked_line *ptr);
    void get_clicked_text(class AW_clicked_text *ptr);
    
    int box(int gc, bool filled, AW_pos x0,AW_pos y0,AW_pos width,AW_pos heigth, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
        return generic_box(gc, filled, x0, y0, width, heigth, filteri, cd1, cd2);
    }
    int filled_area(int gc, int npoints, AW_pos *points, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
        return generic_filled_area(gc, npoints, points, filteri, cd1, cd2);
    }
    int circle(int gc, bool filled, AW_pos x0,AW_pos y0,AW_pos width,AW_pos heigth, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
        return generic_circle(gc, filled, x0, y0, width, heigth, filteri, cd1, cd2);
    }    
    int arc(int gc, bool filled, AW_pos x0,AW_pos y0,AW_pos width,AW_pos heigth, int start_degrees, int arc_degrees, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
        return generic_arc(gc, filled, x0, y0, width, heigth, start_degrees, arc_degrees, filteri, cd1, cd2);
    }
};

#else
#error aw_click.hxx included twice
#endif