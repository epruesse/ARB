#ifndef AW_XM_HXX
#define AW_XM_HXX

#ifndef AW_DEVICE_HXX
#include <aw_device.hxx>
#endif

class AW_device_Xm : public AW_device {
    int fastflag;

    int line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri);
    int text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen);
    int box_impl(int gc, bool filled, const AW::Rectangle& rect, AW_bitset filteri);
    int circle_impl(int gc, bool filled, const AW::Position& center, AW_pos xradius, AW_pos yradius, AW_bitset filteri);
    int arc_impl(int gc, bool filled, const AW::Position& center, AW_pos xradius, AW_pos yradius, int start_degrees, int arc_degrees, AW_bitset filter);
    int filled_area_impl(int gc, int npos, const AW::Position *pos, AW_bitset filteri) {
        return generic_filled_area(gc, npos, pos, filteri);
    }

public:
    AW_device_Xm(AW_common *commoni);

    void           init();
    AW_DEVICE_TYPE type();

    void clear(AW_bitset filteri);
    void clear_part(AW_pos x, AW_pos y, AW_pos width, AW_pos height, AW_bitset filteri);
    void clear_text(int gc, const char *string, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL cd1, AW_CL cd2);

    void fast();                // e.g. zoom linewidth off
    void slow();
    void flush();
    void move_region(AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height, AW_pos dest_x, AW_pos dest_y);
};

#else
#error aw_Xm.hxx included twice
#endif
