#ifndef AW_XM_HXX
#define AW_XM_HXX


class AW_device_Xm: public AW_device {
    int fastflag;
public:
    AW_device_Xm(AW_common *commoni);
    
    void           init(void);
    AW_DEVICE_TYPE type(void);

    int line(int gc, AW_pos x0,AW_pos y0, AW_pos x1,AW_pos y1, AW_bitset filter, AW_CL cd1, AW_CL cd2);
    int text(int gc, const char *string,AW_pos x,AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL cd1, AW_CL cd2,long opt_strlen);
    int box(int gc, AW_BOOL filled, AW_pos x0,AW_pos y0,AW_pos width,AW_pos heigth, AW_bitset filter, AW_CL cd1, AW_CL cd2);
    int circle(int gc, AW_BOOL filled, AW_pos x0,AW_pos y0,AW_pos width,AW_pos heigth, AW_bitset filter, AW_CL cd1, AW_CL cd2);
    int arc(int gc, AW_BOOL filled, AW_pos x0,AW_pos y0,AW_pos width,AW_pos heigth, int start_degrees, int arc_degrees, AW_bitset filter, AW_CL cd1, AW_CL cd2);

    void clear(AW_bitset filteri);
    void clear_part(AW_pos x, AW_pos y,AW_pos width, AW_pos height, AW_bitset filteri);
    void clear_text(int gc, const char *string, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL cd1, AW_CL cd2);
    
    void fast(void);            // e.g. zoom linewidth off
    void slow(void);
    void flush(void);
    void move_region( AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height, AW_pos dest_x, AW_pos dest_y );
};

#else
#error aw_Xm.hxx included twice
#endif
