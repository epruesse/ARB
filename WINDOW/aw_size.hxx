// =============================================================== //
//                                                                 //
//   File      : aw_size.hxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AW_SIZE_HXX
#define AW_SIZE_HXX

#ifndef AW_DEVICE_HXX
#include "aw_device.hxx"
#endif
#ifndef AW_BASE_HXX
#include "aw_base.hxx"
#endif


class AW_device_size : public AW_device {
    bool     drawn;
    AW_world size_information;
    void     privat_reset();

    void dot(AW_pos x, AW_pos y);
    void dot_transformed(AW_pos X, AW_pos Y);

public:
    AW_device_size(AW_common *commoni);

    void           init();
    AW_DEVICE_TYPE type();

    bool invisible(int gc, AW_pos x, AW_pos y, AW_bitset filteri, AW_CL cd1, AW_CL cd2);
    int  line(int gc, AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_bitset filteri, AW_CL cd1, AW_CL cd2);
    int  text(int gc, const  char *string, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL cd1, AW_CL cd2, long opt_strlen = 0);

    void get_size_information(AW_world *ptr);

    int box(int gc, bool filled, AW_pos x0, AW_pos y0, AW_pos width, AW_pos heigth, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
        return generic_box(gc, filled, x0, y0, width, heigth, filteri, cd1, cd2);
    }
    int filled_area(int gc, int npoints, AW_pos *points, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
        return generic_filled_area(gc, npoints, points, filteri, cd1, cd2);
    }
    int circle(int gc, bool filled, AW_pos x0, AW_pos y0, AW_pos width, AW_pos heigth, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
        return generic_circle(gc, filled, x0, y0, width, heigth, filteri, cd1, cd2);
    }
    int arc(int gc, bool filled, AW_pos x0, AW_pos y0, AW_pos width, AW_pos heigth, int start_degrees, int arc_degrees, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
        return generic_arc(gc, filled, x0, y0, width, heigth, start_degrees, arc_degrees, filteri, cd1, cd2);
    }
};

#else
#error aw_size.hxx included twice
#endif // AW_SIZE_HXX
