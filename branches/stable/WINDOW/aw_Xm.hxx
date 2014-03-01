#ifndef AW_XM_HXX
#define AW_XM_HXX

#ifndef AW_COMMON_XM_HXX
#include "aw_common_xm.hxx"
#endif

class AW_device_Xm : public AW_device {
    bool line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri);
    bool text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen);
    bool box_impl(int gc, bool filled, const AW::Rectangle& rect, AW_bitset filteri);
    bool circle_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, AW_bitset filteri);
    bool arc_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, int start_degrees, int arc_degrees, AW_bitset filter);
    bool filled_area_impl(int gc, int npos, const AW::Position *pos, AW_bitset filteri) {
        return generic_filled_area(gc, npos, pos, filteri);
    }
    bool invisible_impl(const AW::Position& pos, AW_bitset filteri) { return generic_invisible(pos, filteri); }

    void specific_reset() {}
    
public:
    AW_device_Xm(AW_common *commoni)
        : AW_device(commoni)
    {}

    AW_common_Xm *get_common() const { return DOWNCAST(AW_common_Xm*, AW_device::get_common()); }

    AW_DEVICE_TYPE type();

    void clear(AW_bitset filteri);
    void clear_part(const AW::Rectangle& rect, AW_bitset filteri);

    void flush();
    void move_region(AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height, AW_pos dest_x, AW_pos dest_y);
};

#else
#error aw_Xm.hxx included twice
#endif
