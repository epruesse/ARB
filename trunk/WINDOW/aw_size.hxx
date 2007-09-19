#ifndef AW_SIZE_HXX
#define AW_SIZE_HXX


class AW_device_size: public AW_device {
    bool     drawn;
    AW_world size_information;
    void     privat_reset(void);

    void dot(AW_pos x, AW_pos y);
    void dot_transformed(AW_pos X, AW_pos Y);

public:
    AW_device_size(AW_common *commoni);

    void           init(void);
    AW_DEVICE_TYPE type(void);

    bool invisible(int gc, AW_pos x, AW_pos y, AW_bitset filteri, AW_CL cd1, AW_CL cd2);
    int  line(int gc, AW_pos x0,AW_pos y0, AW_pos x1,AW_pos y1, AW_bitset filteri, AW_CL cd1, AW_CL cd2);
    int  text(int gc,const  char *string,AW_pos x,AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL cd1, AW_CL cd2,long opt_strlen = 0);
    
    void get_size_information(AW_world *ptr);
};

#else
#error aw_size.hxx included twice
#endif
